#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "ResourcePath.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace sf;
using namespace std;

#define PORT 8888

mutex mtx;
condition_variable cv;
bool used = false;

int size = 94;
Vector2f offset(47, 47);

Sprite f[32]; //figures
string position = "";
string turn = "";
string step = "";
string color = "";

int white_board[8][8] = {
    -1,-2,-3,-4,-5,-3,-2,-1,
    -6,-6,-6,-6,-6,-6,-6,-6,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    6, 6, 6, 6, 6, 6, 6, 6,
    1, 2, 3, 4, 5, 3, 2, 1
};

int black_board[8][8] = {
    1, 2, 3, 5, 4, 3, 2, 1,
    6, 6, 6, 6, 6, 6, 6, 6,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -6,-6,-6,-6,-6,-6,-6,-6,
    -1,-2,-3,-5,-4,-3,-2,-1,
};

string toChessNote(Vector2f p)
{
    string s = "";
    if(color == "white") {
        s += char(p.x / size + 'a');
        s += char(7 - p.y / size + '1');
    } else {
        s += char(7 - p.x / size + 'a');
        s += char(p.y / size + '1');
    }
    return s;
}

Vector2f toCoord(char a, char b)
{
    int x, y;
    if(color == "white") {
        x = int(a) - 'a';
        y = 7 - int(b) + '1';
    } else {
        x = 7 - int(a) + 'a';
        y = int(b) - '1';
    }
    return Vector2f(x * size, y * size);
}

void move(string str)
{
    Vector2f oldPos = toCoord(str[0], str[1]);
    Vector2f newPos = toCoord(str[2], str[3]);
    
    for(int i = 0; i < 32; i++)
        if (f[i].getPosition() == newPos)
            f[i].setPosition(-500,-500);
    
    for(int i = 0; i < 32; i++)
        if (f[i].getPosition() == oldPos)
            f[i].setPosition(newPos);
    
    SoundBuffer buffer;
    buffer.loadFromFile(resourcePath() + "piece_move_sound.wav");
    Sound sound;
    sound.setBuffer(buffer);
    sound.play();
    
    //castling       //if the king didn't move
    if (str == "e1g1") if (position.find("e1") == -1) move("h1f1");
    if (str == "e8g8") if (position.find("e8") == -1) move("h8f8");
    if (str == "e1c1") if (position.find("e1") == -1) move("a1d1");
    if (str == "e8c8") if (position.find("e8") == -1) move("a8d8");
}

void loadPosition()
{
    int k = 0;
    for(int i = 0; i < 8; i++)
        for(int j = 0; j < 8; j++)
        {
            int n = color == "white" ? white_board[i][j] : black_board[i][j];
            if (!n) continue;
            int x = abs(n) - 1;
            int y = n > 0 ? 1 : 0;
            f[k].setTextureRect( IntRect(size * x,size * y, size, size) );
            f[k].setPosition(size * j, size * i);
            k++;
        }
    
    for(int i = 0; i < position.length(); i += 5)
        move( position.substr(i, 4) );
}

inline bool canMove(string step) {
    
    return true;
}

string receive(int socket) {
    char buf[1025];
    int n = recv(socket, buf, 1024, 0);
    if(n < 0) return "ERROR";
    
    string res = "";
    for(int i = 0; i < strlen(buf); ++i)
        res += buf[i];
    return res;
}

void listener(int server) {
    while(true) {
        string temp = receive(server);

        unique_lock<mutex> lck(mtx);
        while(used) cv.wait(lck);
        used = true;
        
        cerr << "lets receive\n";
        step = temp;
        move(step);
        turn = "GO";
        cerr << "received " << turn << "\n";
        
        used = false;
        cv.notify_one();
    }
}

int main(int argc, char *argv[]) {
    
    int option;
    string host = "localhost";
    
    // use DNS to get IP address
    struct hostent *hostEntry;
    hostEntry = gethostbyname(host.c_str());
    if (!hostEntry) {
        cout << "No such host name: " << host << endl;
        exit(-1);
    }
    
    // setup socket address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);
    
    // create socket
    int server = socket(PF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
    }
    
    // connect to server
    if (connect(server, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(-1);
    }
    
    color = receive(server);
    cout << "my color is: " << color << "\n";
    turn = color == "white" ? "GO" : "STOP";
    
    string ready = "ready";
    send(server, ready.c_str(), ready.size() + 1, 0);
    
    int n = 0;
    
    thread thr = thread(listener, server);
    
    bool isMove = false;
    double dx = 0, dy = 0;
    Vector2f oldPos, newPos;
    
    RenderWindow window(VideoMode(850, 850), "Chess");
    Texture t1, t2;
    t1.loadFromFile(resourcePath() + "figures.png");
    t2.loadFromFile(resourcePath() + "board.png");
    for(int i = 0; i < 32; i++)
        f[i].setTexture(t1);
    Sprite sBoard(t2);
    
    loadPosition();
    
    while(window.isOpen())
    {
        
        Vector2i pos = Mouse::getPosition(window) - Vector2i(offset);
        
        Event e;
        while (window.pollEvent(e))
        {
            if (e.type == Event::Closed)
                window.close();
            /////drag and drop///////
            if (e.type == Event::MouseButtonPressed)
            {
                if (e.key.code == Mouse::Left) {
                    for(int i = 0; i < 32; i++)
                        if (f[i].getGlobalBounds().contains(pos.x, pos.y))
                        {
                            isMove = true;
                            n = i;
                            dx = pos.x - f[i].getPosition().x;
                            dy = pos.y - f[i].getPosition().y;
                            oldPos = f[i].getPosition();
                        }
                }
            }
            if (e.type == Event::MouseButtonReleased)
            {
                if (e.key.code == Mouse::Left)
                {
                    isMove = false;
                    Vector2f p = f[n].getPosition() + Vector2f(size / 2, size / 2);
                    newPos = Vector2f( size * int(p.x / size), size * int(p.y / size) );
                    if(toChessNote(oldPos) == toChessNote(newPos)) {
                        f[n].setPosition(oldPos);
                        continue;
                    }
                    
                    unique_lock<mutex> lck(mtx);
                    while(used) cv.wait(lck);
                    used = true;
                    
                    step = toChessNote(oldPos) + toChessNote(newPos);
                    cerr << step << endl;
                    
                    if(turn == "GO") {
                        if(canMove(step)) {
                            move(step);
                            send(server, step.c_str(), step.size() + 1, 0);
                            position += step + " ";
                            f[n].setPosition(newPos);
                            turn = "STOP";
                        } else {
                            cout << "incorrect movement\n";
                        }
                    } else {
                        cout << "not your turn\n";
                        f[n].setPosition(oldPos);
                    }
                    
                    used = false;
                    cv.notify_one();
                }
            }
        }
        if (isMove) f[n].setPosition(pos.x - dx, pos.y - dy);
    
        ////// draw  ///////
        window.clear();
        window.draw(sBoard);
        for(int i = 0; i < 32; i++) f[i].move(offset);
        for(int i = 0; i < 32; i++) window.draw(f[i]); window.draw(f[n]);
        for(int i = 0; i < 32; i++) f[i].move(-offset);
        window.display();
    }    
    // Close socket
    close(server);
    
    return EXIT_SUCCESS;
}
