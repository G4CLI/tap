#include "ncursesw/ncurses.h"
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <ctime>
#include <vector>

#include "config.h"

#include <locale.h>

char gc(){
    char buf=0;
    struct termios old={0};
    fflush(stdout);
    if(tcgetattr(0, &old)<0)
        perror("tcsetattr()");
    old.c_lflag&=~ICANON;
    old.c_lflag&=~ECHO;
    old.c_cc[VMIN]=1;
    old.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &old)<0)
        perror("tcsetattr ICANON");
    if(read(0,&buf,1)<0)
        perror("read()");
    old.c_lflag|=ICANON;
    old.c_lflag|=ECHO;
    if(tcsetattr(0, TCSADRAIN, &old)<0)
        perror("tcsetattr ~ICANON");
    return buf;
}

std::string rMap(int len){
    // TODO: check max top, bottom
    srand(time (0));
    std::string rMap = "__________";
    char newChar;
    for (int i = 0; i < len - 10; ++i){
        int nChar = (rand() % static_cast<int>(101));
        if (nChar < 5){
            newChar = '/';
        } else if (nChar < 10){
            newChar = 'v';
        } else {
            newChar = '_';
        }
        rMap += newChar;
    }
    return rMap;
}

struct gPlayer{
    bool jumping = false;
    bool alive = true;
    int jump = 0;  // Add to Y
    int X = 1;    // End of level map freeze
    int Y = 4;   // Dinamic with terminal size and jump
} mainPlayer;

int main(){
    setlocale(LC_ALL,"");              // Allow unicode characters like the box drawing characters for the floor
    // If arg int then maplen arg
    int maplen = 500;
    // If arg str then map file str
    std::string map = rMap(maplen);
    std::vector<std::string> pPlayer = {"◐", "◓", "◑", "◒"};
    int pp = 0;
    int screens = 0;
    std::string currentDebugText = "None";
    const int pause = 10;           // Pause so game time = refresh * n
    int floorLevel = 3;            // Floor level
    int floorElev = 0;            // Floor elevation
    int floorPlus = 0;           // Trying to have a static base, prob temporal
    struct winsize w;           // Terminal size
    int timer = 0;             // For pause
    bool goingup;             // Jump direction
    int gtim = 0;            // Global timer

    // MAIN LOOP
    initscr();
    curs_set(0);
    while(mainPlayer.alive){                     // While player is alive

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);  // Terminal size

        std::thread thread([&thread]() {
            gc();
            thread.detach();               // Wait for input
        });

        // IDLE LOOP
        while(thread.joinable()){
            if (timer >= pause){
                if (pp >= 3){
                    pp = 0;
                } else {
                    pp++;
                }
                floorElev = 0;
                std::string visibleMap(map.c_str() + gtim, map.c_str() + maplen);
                clear();
                timer = 0;
                if (graph){
                    screens++;
                }
                // CHECK FOR END
                if (visibleMap.empty()){
                    goto end;
                }
                // DRAW SCREEN
                for (int i = 0; i < visibleMap.length(); ++i){

                    if (graph){ // GRAPH
                        mvprintw(1,1,"floorPlus         %d", floorPlus * -1);
                        mvprintw(2,1,"mainPlayer.jump   %d", mainPlayer.jump);
                        mvprintw(3,1,"mainPlayer.Y      %d", mainPlayer.Y);
                        mvprintw(4,1,"visibleMap[0]     %c", visibleMap[0]);
                        mvprintw(5,1,"pPlayer pp        %d", pp);
                        mvprintw(6,1,"screens           %d", screens);
                        mvprintw(7,1,"currentDebugText  %s", currentDebugText.c_str());
                    }

                    if (visibleMap[i] == '/'){
                        mvprintw(w.ws_row - (floorLevel + floorElev) + floorPlus, i, "\u251b");
                        floorElev++;
                        mvprintw(w.ws_row - (floorLevel + floorElev) + floorPlus, i, "\u250F");
                    } else if (visibleMap[i] == 'v'){
                        mvprintw(w.ws_row - (floorLevel + floorElev) + floorPlus, i, "\u2513");
                        floorElev--;
                        mvprintw(w.ws_row - (floorLevel + floorElev) + floorPlus, i, "\u2517");
                    } else {
                        mvprintw(w.ws_row - (floorLevel + floorElev) + floorPlus, i, "\u2501");
                    }

                }
                if (visibleMap[0] == '/'){
                    floorPlus--;
                    if (mainPlayer.Y + mainPlayer.jump <= -111){ // temporal
                        goto end;
                    }
                } else if (visibleMap[0] == 'v'){
                    if (!mainPlayer.jumping){
                        mainPlayer.Y--;
                    }
                    floorPlus++;
                }

                if ( mainPlayer.Y + mainPlayer.jump - 4 < floorPlus * -1 ){
                    goto end;
                }
                mvprintw(w.ws_row - (mainPlayer.Y + mainPlayer.jump), 1, pPlayer[pp].c_str()); // Player
                // JUMP HERE
                if (mainPlayer.jumping) {
                    // ^
                    if (mainPlayer.jump < 5 && goingup){
                        mainPlayer.jump++;
                        currentDebugText = "JumpTriggered";
                    // v start
                    } else if (mainPlayer.Y + mainPlayer.jump > (floorPlus * -1) + 4){
                        mainPlayer.jump--;
                        goingup = false;
                        currentDebugText = "Falling";
                    // v continue, check floor
                    } else {
                        mainPlayer.Y += mainPlayer.jump;
                        mainPlayer.jump = 0;
                        mainPlayer.jumping = false;
                        currentDebugText = "FloorBelow";
                    }
                }
                gtim++;
                refresh();
            }
            timer++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // POOL ELDI

        // JUMP HERE
        if ( ! mainPlayer.jumping ){
            mainPlayer.jump++;
            goingup = true;
        }
        mainPlayer.jumping = true;
    }
end:
    endwin();
    curs_set(1);
    // POOL NIAM

    return 0;
}