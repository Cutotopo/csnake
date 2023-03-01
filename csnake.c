// CSnake
// https://github.com/Cutotopo/csnake

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gtk/gtk.h>
#define FIELD_HEIGHT 25
#define FIELD_WIDTH 25

typedef struct Player {
    int position[FIELD_HEIGHT][FIELD_WIDTH];
    int direction; // valid values are 0: up, 1: right, 2: down, 3: left
    int maxValue;
    int isGrowing;
} player;

typedef struct Snake {
    int score;
    int field[FIELD_HEIGHT][FIELD_WIDTH];
    player snake;
    int isSnakePlaced;
    int isApplePlaced;
    int level;
    int gameFieldRefreshTimeout;
    int isGameOver;
} snake;

// game field widgets
GtkWidget *gameFieldSquare[FIELD_HEIGHT][FIELD_WIDTH];
// top label widget
GtkWidget *gameStateLabel;
// window
GtkWidget *window;
// game
snake game;

// quit the application
void quitApplication() {
      gtk_window_close(GTK_WINDOW(window));
}

// set window contents to game over
void finishGame() {
    GtkWidget *gameOverBox;
    gameOverBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(gameOverBox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gameOverBox, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), gameOverBox);
    GtkWidget *gameOverLabel;
    gameOverLabel = gtk_label_new("Game over");
    gtk_widget_add_css_class(gameOverLabel, "gameOverTitle");
    gtk_box_append(GTK_BOX(gameOverBox), gameOverLabel);
    GtkWidget *gameOverDetails;
    gameOverDetails = gtk_label_new("Loading details...");
    gtk_box_append(GTK_BOX(gameOverBox), gameOverDetails);
    char scoreLabel[100];
    sprintf(scoreLabel, "Score: %d - Level: %d", game.score, game.level);
    gtk_label_set_label(GTK_LABEL(gameOverDetails), scoreLabel);
    GtkWidget *gameOverExitButton;
    gameOverExitButton = gtk_button_new_with_label ("Quit");
    gtk_widget_add_css_class(gameOverExitButton, "gameOverExitButton");
    g_signal_connect(gameOverExitButton, "clicked", G_CALLBACK (quitApplication), NULL);
    gtk_box_append(GTK_BOX(gameOverBox), gameOverExitButton);
}

// set direction label
char* generateDirectionLabel(int direction) {
    switch(direction) {
        case 0:
            return "North";
            break;
        case 1:
            return "East";
            break;
        case 2:
            return "South";
            break;
        case 3:
            return "West";
            break;
    }
}

// set game level
void checkLevelScore() {
    game.level = (game.score / 5) + 1;
}

// update label content
void updateLabels() {
    char scoreLabel[100];
    sprintf(scoreLabel, "Score: %d - Level: %d - Direction: %s", game.score, game.level, generateDirectionLabel(game.snake.direction));
    gtk_label_set_label(GTK_LABEL(gameStateLabel), scoreLabel);
}

// find value in matrix
void findValueCoordinatesInMatrix(int matrix[FIELD_HEIGHT][FIELD_WIDTH], int value, int* coordinates) {
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH; j++) {
            if (matrix[i][j] == value) {
                coordinates[0] = i;
                coordinates[1] = j;
            }
        }
    }
}

// handle keypressed event
gboolean key_pressed(GtkEventControllerKey* self, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    switch (keycode) {
        case 111:
            if (game.snake.direction != 2) {
                game.snake.direction = 0;
            }
            break;
        case 116:
            if (game.snake.direction != 0) {
                game.snake.direction = 2;
            }
            break;
        case 113:
            if (game.snake.direction != 1) {
                game.snake.direction = 3;
            }
            break;
        case 114:
            if (game.snake.direction != 3) {
                game.snake.direction = 1;
            }
            break;
    }
    return TRUE;
}

// empty game field
void emptyField() {
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH; j++) {
            game.field[i][j] = 0;
        }
    }
}

// refreshes the game field
gboolean refreshField() {
    // if the apple was not placed, it is now placed
    if (!game.isApplePlaced) {
        int appleX;
        int appleY;
        do {
            appleX = rand()%FIELD_WIDTH;
            appleY = rand()%FIELD_HEIGHT;
        } while (game.snake.position[appleY][appleX] > 0);
        game.field[appleY][appleX] = 1;
        game.isApplePlaced = 1;
    }

    // if the player was not placed, it its now placed
    if (!game.isSnakePlaced) {
        game.snake.position[FIELD_HEIGHT / 2][FIELD_WIDTH / 2] = 1;
        game.snake.maxValue = 1;
        game.isSnakePlaced = 1;
        game.snake.direction = 1;
        game.snake.isGrowing = 0;
    }
    
    if (game.isSnakePlaced) {
        int snakeHeadCoordinates[2];
        findValueCoordinatesInMatrix(game.snake.position, game.snake.maxValue, snakeHeadCoordinates);
        // check field boundaries
        if (snakeHeadCoordinates[0] > 0 && game.snake.direction == 0) {
            if (game.field[snakeHeadCoordinates[0] - 1][snakeHeadCoordinates[1]] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[snakeHeadCoordinates[0] - 1][snakeHeadCoordinates[1]] > 0) {
                game.isGameOver = 1;
            }
        }
        if (snakeHeadCoordinates[1] > 0 && game.snake.direction == 1) {
            if (game.field[snakeHeadCoordinates[0]][snakeHeadCoordinates[1] + 1] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1] + 1] > 0) {
                game.isGameOver = 1;
            }
        }
        if (snakeHeadCoordinates[0] < FIELD_HEIGHT - 1 && game.snake.direction == 2) {
            if (game.field[snakeHeadCoordinates[0] + 1][snakeHeadCoordinates[1]] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[snakeHeadCoordinates[0] + 1][snakeHeadCoordinates[1]] > 0) {
                game.isGameOver = 1;
            }
        }
        if (snakeHeadCoordinates[1] < FIELD_WIDTH - 1 && game.snake.direction == 3) {
            if (game.field[snakeHeadCoordinates[0]][snakeHeadCoordinates[1] - 1] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1] - 1] > 0) {
                game.isGameOver = 1;
            }
        }
        if (!game.snake.isGrowing) {
            switch(game.snake.direction) {
                case 0:
                    if (snakeHeadCoordinates[0] > 0) {
                        game.snake.position[snakeHeadCoordinates[0] - 1][snakeHeadCoordinates[1]] = game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1]];
                        snakeHeadCoordinates[0]--;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                case 1:
                    if (snakeHeadCoordinates[1] < FIELD_WIDTH - 1) {
                        game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1] + 1] = game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1]];
                        snakeHeadCoordinates[1]++;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                case 2:
                    if (snakeHeadCoordinates[0] < FIELD_HEIGHT - 1) {
                        game.snake.position[snakeHeadCoordinates[0] + 1][snakeHeadCoordinates[1]] = game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1]];
                        snakeHeadCoordinates[0]++;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                case 3:
                    if (snakeHeadCoordinates[1] > 0) {
                        game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1] - 1] = game.snake.position[snakeHeadCoordinates[0]][snakeHeadCoordinates[1]];
                        snakeHeadCoordinates[1]--;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                }
        }
        for (int i = 0; i < FIELD_HEIGHT; i++) {
            for (int j = 0; j < FIELD_WIDTH; j++) {
                if (((i != snakeHeadCoordinates[0] || j != snakeHeadCoordinates[1])) && (game.snake.position[i][j] > 0) && (!game.snake.isGrowing)) {
                    game.snake.position[i][j]--;
                }
            }
        }
        if (game.snake.isGrowing) {
            int appleCoordinates[2];
            game.score++;
            findValueCoordinatesInMatrix(game.field, 1, appleCoordinates);
            game.snake.maxValue++;
            game.snake.position[appleCoordinates[0]][appleCoordinates[1]] = game.snake.maxValue;
            game.field[appleCoordinates[0]][appleCoordinates[1]] = 0;
            game.isApplePlaced = 0;
            game.snake.isGrowing = 0;
        }
    }

    checkLevelScore();
    updateLabels();
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH; j++) {
            switch(game.field[i][j]) {
                case 0:
                    gtk_widget_remove_css_class(gameFieldSquare[i][j], "apple");
                    break;
                case 1:
                    gtk_widget_add_css_class(gameFieldSquare[i][j], "apple");
                    break;
            }

            if (game.snake.position[i][j] > 0) {
                gtk_widget_add_css_class(gameFieldSquare[i][j], "snake");
            } else {
                gtk_widget_remove_css_class(gameFieldSquare[i][j], "snake");
            }

            if (game.snake.position[i][j] == game.snake.maxValue) {
                gtk_widget_add_css_class(gameFieldSquare[i][j], "snakeHead");
            } else {
                gtk_widget_remove_css_class(gameFieldSquare[i][j], "snakeHead");
            }
        }
    }
    if (game.isGameOver) {
        finishGame();
        return FALSE;
    } else {
        return TRUE;
    }
}

// handle activate event
static void activate(GtkApplication* app, gpointer user_data) {
    emptyField();
    game.score = 0;
    game.isGameOver = 0;

    GdkDisplay *display = gdk_display_get_default();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, ".gameOverExitButton { margin-top: 10px; } .gameOverTitle { font-size: 48px; } .gameFieldSquare { border-radius: 0px; } .gameFieldSquare-even { background-color: green; } .gameFieldSquare-odd { background-color: forestgreen; } .apple { background-color: red; } .snake { background-color: blue; } .snakeHead { border: 3px solid yellow; }", -1);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);
    g_object_unref(provider);

    // window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "csnake - https://github.com/Cutotopo/csnake");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    // main ui box
    GtkWidget *mainBox;
    mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkEventController* keyPressEventController = gtk_event_controller_key_new();
    g_signal_connect_object(keyPressEventController, "key-pressed", G_CALLBACK(key_pressed), mainBox, G_CONNECT_SWAPPED);
    gtk_widget_add_controller(GTK_WIDGET(window), keyPressEventController);
    gtk_widget_set_halign(mainBox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(mainBox, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), mainBox);

    // top bar box
    GtkWidget *gameStateBox;
    gameStateBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(gameStateBox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gameStateBox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(mainBox), gameStateBox);

    // game field
    GtkWidget *gameFieldBox;
    gameFieldBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(gameFieldBox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gameFieldBox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(mainBox), gameFieldBox);

    // game field objects
    GtkWidget *gameFieldChildrenBoxes[FIELD_HEIGHT];
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        gameFieldChildrenBoxes[i] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_halign(gameFieldChildrenBoxes[i], GTK_ALIGN_CENTER);
        gtk_widget_set_valign(gameFieldChildrenBoxes[i], GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(gameFieldBox), gameFieldChildrenBoxes[i]);
        for (int j = 0; j < FIELD_WIDTH; j++) {
            gameFieldSquare[i][j] = gtk_label_new("");
            gtk_widget_set_size_request(gameFieldSquare[i][j], 25, 25);
            gtk_widget_add_css_class(gameFieldSquare[i][j], "gameFieldSquare");
            gtk_widget_add_css_class(gameFieldSquare[i][j], (i + j) % 2 == 0 ? "gameFieldSquare-even" : "gameFieldSquare-odd");
            gtk_box_append(GTK_BOX(gameFieldChildrenBoxes[i]), gameFieldSquare[i][j]);
        }
    }

    // about game
    gameStateLabel = gtk_label_new("Starting game...");
    gtk_box_append(GTK_BOX(gameStateBox), gameStateLabel);

    // show the window
    gtk_widget_show(window);

    // use timeout to reload field
    g_timeout_add(game.gameFieldRefreshTimeout, refreshField, NULL);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    game.gameFieldRefreshTimeout = 200;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0)) {
                printf("csnake - https://github.com/Cutotopo/csnake\n===========================================\nOptions:\n  --refreshTimeout / -rt | Set game tick interval in milliseconds (default is 200)\n  --help / -h            | Show this help message\n");
                exit(0);
            }
            if ((strcmp(argv[i], "--refreshTimeout") == 0) || (strcmp(argv[i], "-rt") == 0)) {
                if (atoi(argv[i + 1]) == 0) {
                    printf("Refresh timeout value should be a positive integer.\n");
                    exit(1);
                }
                game.gameFieldRefreshTimeout = atoi(argv[i + 1]);
            }
        }
    }

    // app
    GtkApplication *app;
    int status;

    // app initialization and activation
    app = gtk_application_new("io.github.cutotopo.csnake", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), 0, NULL);

    // quit application
    g_object_unref(app);
    return status;
}
