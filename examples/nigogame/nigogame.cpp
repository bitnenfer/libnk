#include <nk/app.h>
#include <nk/canvas.h>
#include <nk/hid.h>

#include "game/game.h"

int main() {
    NcGame game;
    game.init();
    game.run();
    game.destroy();
    return 0;
}
