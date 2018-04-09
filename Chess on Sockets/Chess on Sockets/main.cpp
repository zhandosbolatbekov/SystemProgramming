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

#define PORT 8080

const char piece_names[] = {'r', 'h', 'b', 'q', 'k', 'p'};

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

char board[8][8];

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
    
    int xa, ya, xb, yb;
    if(color == "white") {
        xa = int(step[0]) - 'a';
        ya = 7 - int(step[1]) + '1';
        xb = int(step[2]) - 'a';
        yb = 7 - int(step[3]) + '1';
    } else {
        xa = 7 - int(step[0]) + 'a';
        ya = int(step[1]) - '1';
        xb = 7 - int(step[2]) + 'a';
        yb = int(step[3]) - '1';
    }
    swap(xa, ya); swap(xb, yb);
    board[xb][yb] = board[xa][ya];
    board[xa][ya] = 0;
    
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
            board[i][j] = piece_names[x];
            if(n < 0) board[i][j] = toupper(board[i][j]);
            f[k].setTextureRect( IntRect(size * x,size * y, size, size) );
            f[k].setPosition(size * j, size * i);
            k++;
        }
    
    for(int i = 0; i < position.length(); i += 5)
        move( position.substr(i, 4) );
}

inline bool isTeammate(int xa, int ya, int xb, int yb) {
    if(!board[xb][yb]) return false;
    if(islower(board[xa][ya]) && islower(board[xb][yb]))
        return true;
    if(isupper(board[xa][ya]) && isupper(board[xb][yb]))
        return true;
    return false;
}

inline bool isEmpty(int x, int y) { return board[x][y] == 0; }
inline bool isRook(int x, int y) { return tolower(board[x][y]) == 'r'; }

inline bool isKnight(int x, int y) { return tolower(board[x][y]) == 'h'; }
int knight_x[] = {-1, -2, -2, -1, 1, 2, 2, 1};
int knight_y[] = {-2, -1, 1, 2, 2, 1, -1, -2};

inline bool isBishop(int x, int y) { return tolower(board[x][y]) == 'b'; }
inline bool isQueen(int x, int y) { return tolower(board[x][y]) == 'q'; }
inline bool isKing(int x, int y) { return tolower(board[x][y]) == 'k'; }
int king_x[] = {-1, -1, -1, 0, 1, 1, 1, 0};
int king_y[] = {-1, 0, 1, 1, 1, 0, -1, -1};

inline bool isPawn(int x, int y) { return tolower(board[x][y]) == 'p'; }
inline bool canMove(string step);
inline bool canMove(int xa, int ya, int xb, int yb);

inline bool canReach(int xa, int ya, int xb, int yb) {
    switch (tolower(board[xa][ya])) {
        case 'r':
        {
            if(xa != xb && ya != yb) return false;
            if(xa == xb) {
                for(int i = min(ya, yb) + 1; i < max(ya, yb); ++i)
                    if(!isEmpty(xa, i)) return false;
            } else {
                for(int i = min(xa, xb) + 1; i < max(xa, xb); ++i)
                    if(!isEmpty(i, ya)) return false;
            }
            return true;
            break;
        }
        case 'h':
        {
            for(int i = 0; i < 8; ++i)
                if(xa + knight_x[i] == xb && ya + knight_y[i] == yb) {
                    return true;
                }
            return false;
            break;
        }
        case 'b':
        {
            if(xa + ya != xb + yb && xa - ya != xb - yb) return false;
            int dx = 0, dy = 0;
            if(xa < xb) dx = 1; else dx = -1;
            if(ya < yb) dy = 1; else dy = -1;
            for(int x = xa + dx, y = ya + dy; x != xb && y != yb; x += dx, y += dy)
                if(!isEmpty(x, y)) return false;
            return true;
            break;
        }
        case 'q':
        {
            if(xa == xb || ya == yb) {
                if(xa != xb && ya != yb) return false;
                if(xa == xb) {
                    for(int i = min(ya, yb) + 1; i < max(ya, yb); ++i)
                        if(!isEmpty(xa, i)) return false;
                } else {
                    for(int i = min(xa, xb) + 1; i < max(xa, xb); ++i)
                        if(!isEmpty(i, ya)) return false;
                }
                return true;
            } else if(xa + ya == xb + yb || xa - ya == xb - yb) {
                int dx = 0, dy = 0;
                if(xa < xb) dx = 1; else dx = -1;
                if(ya < yb) dy = 1; else dy = -1;
                for(int x = xa + dx, y = ya + dy; x != xb && y != yb; x += dx, y += dy)
                    if(!isEmpty(x, y)) return false;
                return true;
            } else {
                return false;
            }
            break;
        }
        case 'p':
        {
            if(xa - 1 == xb && ya == yb) {
                if(!isEmpty(xb, yb)) return false;
                return true;
            } else if(xa - 1 == xb && ya - 1 == yb) {
                if(isEmpty(xb, yb)) return false;
                return true;
            } else if(xa - 1 == xb && ya + 1 == yb) {
                if(isEmpty(xb, yb)) return false;
                return true;
            } else if(xa - 2 == xb && ya == yb && xb >= 4) {
                if(!isEmpty(xa - 1, ya)) return false;
                if(!isEmpty(xa - 2, ya)) return false;
                return true;
            } else {
                return false;
            }
            break;
        }
        case 'k':
        {
            for(int i = 0; i < 8; ++i)
                if(xa + king_x[i] == xb && ya + king_y[i] == yb) {
                    return true;
                }
            return false;
        }
        default:
            return false;
            break;
    }
}

inline bool amIChecked()
{
    if(color == "white")
    {
        for(int i = 0; i < 8; ++i) {
            for(int j = 0; j < 8; ++j) {
                if(islower(board[i][j]) && isKing(i, j)) {
                    for(int x = 0; x < 8; ++x) {
                        for(int y = 0; y < 8; ++y) {
                            if(isupper(board[x][y]) && canReach(x, y, i, j)) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        for(int i = 0; i < 8; ++i) {
            for(int j = 0; j < 8; ++j) {
                if(isupper(board[i][j]) && isKing(i, j)) {
                    for(int x = 0; x < 8; ++x) {
                        for(int y = 0; y < 8; ++y) {
                            if(islower(board[x][y]) && canReach(x, y, i, j)) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

inline bool iAmMated()
{
    if(!amIChecked()) return false;
    if(color == "white") {
        for(int i = 0; i < 8; ++i) {
            for(int j = 0; j < 8; ++j) {
                if(islower(board[i][j])) {
                    for(int x = 0; x < 8; ++x) {
                        for(int y = 0; y < 8; ++y) {
                            if(canMove(i, j, x, y))
                                return false;
                        }
                    }
                }
            }
        }
        return true;
    } else {
        for(int i = 0; i < 8; ++i) {
            for(int j = 0; j < 8; ++j) {
                if(isupper(board[i][j])) {
                    for(int x = 0; x < 8; ++x) {
                        for(int y = 0; y < 8; ++y) {
                            if(canMove(i, j, x, y))
                                return false;
                        }
                    }
                }
            }
        }
        return true;
    }
}

inline bool iResque(int xa, int ya, int xb, int yb)
{
    int copy_board[8][8];
    for(int i = 0; i < 8; ++i)
        for(int j = 0; j < 8; ++j)
            copy_board[i][j] = board[i][j];
    board[xb][yb] = board[xa][ya];
    board[xa][ya] = 0;
    bool checked = amIChecked();
    for(int i = 0; i < 8; ++i)
        for(int j = 0; j < 8; ++j)
            board[i][j] = copy_board[i][j];
    return !checked;
}

inline bool canMove(int xa, int ya, int xb, int yb) {
    swap(xa, xb);
    swap(ya, yb);
    string step = "";
    if(color == "white") {
        step += xa + 'a';
        step += 7 - ya + '1';
        step += xb + 'a';
        step += 7 - yb + '1';
    } else {
        step += 7 - xa + 'a';
        step += ya + '1';
        step += 7 - xb + 'a';
        step += yb + '1';
    }
    return canMove(step);
}

inline bool canMove(string step) {
    int xa, ya, xb, yb;
    if(color == "white") {
        xa = int(step[0]) - 'a';
        ya = 7 - int(step[1]) + '1';
        xb = int(step[2]) - 'a';
        yb = 7 - int(step[3]) + '1';
    } else {
        xa = 7 - int(step[0]) + 'a';
        ya = int(step[1]) - '1';
        xb = 7 - int(step[2]) + 'a';
        yb = int(step[3]) - '1';
    }
    swap(xa, ya); swap(xb, yb);
    cerr << xa << " " << ya << " " << xb << " " << yb << endl;
    
    if(color == "white" && isupper(board[xa][ya])) return false;
    if(color == "black" && islower(board[xa][ya])) return false;

    if(isTeammate(xa, ya, xb, yb)) return false;
    if(isKing(xb, yb)) return false;
    if(!canReach(xa, ya, xb, yb)) return false;
    return iResque(xa, ya, xb, yb);
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

int sendTCP(int socket, string buf) {
    int n = send(socket, buf.c_str(), buf.size() + 1, 0);
    if(n < 0) {
        cerr << buf << endl;
        printf("send error\n");
        exit(EXIT_FAILURE);
    }
    return n;
}

void listener(int server) {
    while(true) {
        string temp = receive(server);

        unique_lock<mutex> lck(mtx);
        while(used) cv.wait(lck);
        used = true;
        
        cerr << temp << endl;
        
        if(temp.size() == 10) { // "FINISH" + STEP
            step = temp.substr(6, 4);
            turn = "FINISH";
        } else {
            step = temp;
            turn = "GO";
        }
        move(step);
        
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
    for(int i = 0; i < 8; ++i) {
        for(int j = 0; j < 8; ++j) {
            cout << board[i][j] << " ";
        }
        cout << "\n";
    }
    cout << endl;
    
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
                            sendTCP(server, step);
                            position += step + " ";
                            f[n].setPosition(newPos);
                            oldPos = f[n].getPosition();
                            turn = "STOP";
                        } else {
                            cout << "incorrect movement\n";
                            f[n].setPosition(oldPos);
                        }
                    }
                    else {
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
