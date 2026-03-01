#include <game.h>

#include <RC2D/RC2D.h>

#include <game_screen.h>
#include <game_client.h>

GameScreen gameScreen;

void rc2d_unload(void)
{
}
    
void rc2d_load(void)
{
    if (initializeClient() != 0)
    {
        RC2D_log(RC2D_LOG_DEBUG, "Failed to initialize client.");
    }
}

void rc2d_update(double dt)
{
    gameScreen.update(dt);
}

void rc2d_draw(void)
{
    
}

void rc2d_keypressed(const char *key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID)
{

}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{

}