#include <stack>
#include <deque>
#include <queue>
#include <unordered_map>
#include <string.h>
#include <stdlib.h>

#define MAX(a,b) (((a)>(b))?(a):(b))

using namespace std;

template <typename word_t>
class Node
{
    public:
        Node* left;
        Node* right;
        word_t value;
        long counter;
        int level;
        bool is_terminal;

        Node(Node* _left, Node* _right, long _counter, bool _is_terminal, long _value, int _level) :
            left(_left), right(_right), value(_value), counter(_counter), is_terminal(_is_terminal), level(_level) {}
};

template <typename word_t>
struct node_comparator
{
    bool operator()(const Node<word_t>* a, const Node<word_t>* b) const
    {
        return a->counter > b->counter;
    }
};

template <typename word_t>
void generate_tree(word_t* input, long length, Node<word_t>*& tree, int& max_level, int& num_nodes)
{
    unordered_map<word_t, long> frequencies;
    max_level = 0;

    for (long l = 0; l < length; ++l)
        frequencies[input[l]]++;

    num_nodes = frequencies.size();

    priority_queue<Node<word_t>*, deque<Node<word_t>*>, node_comparator<word_t>> subtrees;
    for (auto it = frequencies.begin(); it != frequencies.end(); ++it)
        subtrees.push(new Node<word_t>(NULL, NULL, it->second, true, it->first, 0));

    while (subtrees.size() > 1)
    {
        Node<word_t>* left = subtrees.top();
        subtrees.pop();
        Node<word_t>* right = subtrees.top();
        subtrees.pop();

        int new_level = MAX(left->level, right->level) + 1;
        max_level = MAX(max_level, new_level);

        subtrees.push(new Node<word_t>(left, right, left->counter + right->counter, false, 0, new_level));
    }

    tree = subtrees.top();
}

template <typename word_t>
uint16_t generate_array_tree_representation(word_t* input, long length, word_t*& huffman_array, bool*& terminator_array, Node<word_t>*& tree)
{
    // generate tree
    int max_level, num_nodes;
    generate_tree(input, length, tree, max_level, num_nodes);

    // algorithm taken from Kuo-Liang Chung: Efficient Huffman decoding
    uint16_t array_size = 2 * num_nodes - 1;
    huffman_array = new word_t[array_size];
    terminator_array = new bool[array_size];

    long* nodes_per_level = new long[max_level + 1]();

    // first pass: count nodes per level
    tree->level = 0;
    stack<Node<word_t>*> dfs_stack;
    dfs_stack.push(tree);
    
    while (!dfs_stack.empty())
    {
        Node<word_t>* node = dfs_stack.top();
        dfs_stack.pop();

        nodes_per_level[node->level]++;

        if (!node->is_terminal)
        {
            node->left->level = node->level + 1;
            node->right->level = node->level + 1;

            dfs_stack.push(node->left);
            dfs_stack.push(node->right);
        }
    }

    // second pass: bfs traversal, calculate jump values
    int next = 0;
    long* nodes_processed = new long[max_level + 1]();
    long* terminal_nodes_processed = new long[max_level + 1]();
    queue<Node<word_t>*> bfs_queue;
    bfs_queue.push(tree);

    while(!bfs_queue.empty())
    {
        Node<word_t>* node = bfs_queue.front();
        bfs_queue.pop();

        if (node->is_terminal)
        {
            terminal_nodes_processed[node->level]++;
            huffman_array[next] = node->value;
            terminator_array[next++] = true;
        }
        else
        {
            huffman_array[next] = nodes_processed[node->level] - 2 * terminal_nodes_processed[node->level] + nodes_per_level[node->level];
            terminator_array[next++] = false;

            bfs_queue.push(node->left);
            bfs_queue.push(node->right);
        }

        nodes_processed[node->level]++;
    }
    return array_size;
}

template <typename word_t>
struct inverse_map_builder_state {
    Node<word_t>* node;
    char* bits;
    int count_bits;

    inverse_map_builder_state(Node<word_t>* _node, char* _bits, int _count_bits) :
        node(_node), bits(_bits), count_bits(_count_bits) {}

    inverse_map_builder_state() {}
};

template <typename word_t>
using encoding_dict = unordered_map<word_t, struct inverse_map_builder_state<word_t>>;

template <typename word_t>
void build_inverse_mapping(Node<word_t>* tree, encoding_dict<word_t>& encoding_dict)
{
    struct inverse_map_builder_state<word_t> state(tree, NULL, 0);
    stack<struct inverse_map_builder_state<word_t>> traversal_state;
    traversal_state.push(state);

    while (!traversal_state.empty())
    {
        state = traversal_state.top();
        traversal_state.pop();

        if (state.node->is_terminal)
        {
            // input has only one word (possibly repeatedly)
            state.bits = new char[1] {0};
            state.count_bits = 1;
            encoding_dict[state.node->value] = state;
            break;
        }

        struct inverse_map_builder_state<word_t> left(state.node->left, new char[state.count_bits + 1],
                state.count_bits + 1);
        if (state.count_bits > 0)
            memcpy(left.bits, state.bits, state.count_bits);
        left.bits[state.count_bits] = 0;
        if (left.node->is_terminal)
            encoding_dict[left.node->value] = left;
        else
            traversal_state.push(left);

        struct inverse_map_builder_state<word_t> right(state.node->right, new char[state.count_bits + 1],
                state.count_bits + 1);
        if (state.count_bits > 0)
            memcpy(right.bits, state.bits, state.count_bits);
        right.bits[state.count_bits] = 1;
        if (right.node->is_terminal)
            encoding_dict[right.node->value] = right;
        else
            traversal_state.push(right);

        delete state.bits;
    }
}

template <typename word_t>
long encode(word_t* input, long length, char*& output, encoding_dict<word_t>& encoding_dict)
{
    output = new char[length * sizeof(word_t) / sizeof(char)]();

    long byte_pos = 0, bit_pos = 0;
    for (long l = 0; l < length; ++l)
    {
        auto word = encoding_dict[input[l]];

        for (int i = 0; i < word.count_bits; ++i)
        {
            // write a single bit
            output[byte_pos] |= word.bits[i] << (7-bit_pos++);

            if (bit_pos == 8)
            {
                bit_pos = 0;
                ++byte_pos;
            }
        }
    }

    return byte_pos + 1;
}

template <typename word_t>
void decode(char* input, long length, word_t*& output, word_t* huffman_array, bool* terminator_array)
{
    output = new word_t[length];
    long byte_pos = 0, huffman_pos = 0, next = 0;

    while (true)
    {
        unsigned char byte = input[byte_pos];
        
        if(length < 2){
            output[next] = huffman_array[huffman_pos];
            break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += (byte & (unsigned char) 128) >> (unsigned char) 7;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += (byte & (unsigned char) 64) >> (unsigned char) 6;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += (byte & (unsigned char) 32) >> (unsigned char) 5;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += (byte & (unsigned char) 16) >> (unsigned char) 4;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += (byte & (unsigned char) 8) >> (unsigned char) 3;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += (byte & (unsigned char) 4) >> (unsigned char) 2;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += (byte & (unsigned char) 2) >> (unsigned char) 1;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        huffman_pos += huffman_array[huffman_pos];
        huffman_pos += byte & (unsigned char) 1;
        if (terminator_array[huffman_pos]) {
            output[next++] = huffman_array[huffman_pos];
            huffman_pos = 0;
            if (next == length) break;
        }

        byte_pos++;
    }
}

