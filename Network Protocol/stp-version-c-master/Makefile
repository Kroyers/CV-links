client:
	g++ BufferPacket.cpp Connection.cpp Header.cpp Interface.cpp Option.cpp Stp_Client.cpp Transmission.cpp Compression.cpp CongestionControl.cpp Nack.cpp Timers.cpp -lboost_system -lboost_thread -lpthread -o client

server:
	g++ BufferPacket.cpp Connection.cpp Header.cpp Interface.cpp Option.cpp Stp_Server.cpp Transmission.cpp Compression.cpp CongestionControl.cpp Nack.cpp Timers.cpp -lboost_system -lboost_thread -lpthread -o server

clean:
	rm client server