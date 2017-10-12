gcc -o kb_pub -I ../lib/ kb_pub.c -L ../lib/ -lmosquitto -lssl -lcrypto -pthread -lcares
