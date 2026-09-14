// CSnake
// https://github.com/Cutotopo/csnake

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

enum SnakeDirection {
    SNAKE_DIRECTION_UP,
    SNAKE_DIRECTION_RIGHT,
    SNAKE_DIRECTION_DOWN,
    SNAKE_DIRECTION_LEFT
};

enum GameDebugMode {
    DEBUG_MODE_NONE,
    DEBUG_MODE_PRINT_SNAKE,
    DEBUG_MODE_PRINT_FIELD
};

typedef struct Player {
    int **position;
    int position_y;
    int position_x;
    enum SnakeDirection direction; // valid values are 0: up, 1: right, 2: down, 3: left
    int maxValue;
    int isGrowing;
} player;

typedef struct Snake {
    int score;
    int **field;
    int field_width;
    int field_height;
    player snake;
    int isSnakePlaced;
    int apples_placed;
    int apples_target;
    int level;
    int gameFieldRefreshTimeout;
    int isGameOver;
    enum GameDebugMode debug_mode;
} snake;

// bgm gstreamer pipeline and signal
static GstElement *bgm_pipeline = NULL;
static gboolean bgm_started = FALSE;

// game field widgets
GtkWidget ***gameFieldSquare;
// top label widget
GtkWidget *gameStateLabel;
// window
GtkWidget *window;
// game
snake game;

// stop bgm function
void stop_background_music();
// forward declaration for bus callback
static gboolean bgm_bus_callback(GstBus *bus, GstMessage *msg, gpointer data);

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

    // stop the bgm
    stop_background_music();
}

// playing bgm through pipeline
void play_background_music() {
    if (bgm_started) return;

    // make a playbin
    bgm_pipeline = gst_element_factory_make("playbin", "bgm-player");
    if (!bgm_pipeline) {
        g_printerr("Failed to create playbin element.\n");
        return;
    }

    // set the bgm file uri
    gchar *uri = gst_filename_to_uri("skaterswaltz_8bit.mp3", NULL);
    if (!uri) {
        g_printerr("Failed to convert filename to URI.\n");
        gst_object_unref(bgm_pipeline);
        bgm_pipeline = NULL;
        return;
    }
    g_object_set(bgm_pipeline, "uri", uri, NULL);
    g_free(uri);

    // Add bus monitoring to handle errors and end events
    GstBus *bus = gst_element_get_bus(bgm_pipeline);
    gst_bus_add_watch(bus, (GstBusFunc)bgm_bus_callback, NULL);
    gst_object_unref(bus);

    // play
    GstStateChangeReturn ret = gst_element_set_state(bgm_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start playback.\n");
        gst_object_unref(bgm_pipeline);
        bgm_pipeline = NULL;
        return;
    }
    bgm_started = TRUE;
}

// get bus message
static gboolean bgm_bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar *debug = NULL;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("BGM error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_EOS:
            // once bgm end, seek to start to achieve repeat
            if (bgm_pipeline) {
                // FLUSH seek, jump to beginning of bgm
                gst_element_seek_simple(bgm_pipeline, GST_FORMAT_TIME,GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0);

                // avoid the PAUSE status
                // gst_element_set_state(bgm_pipeline, GST_STATE_PLAYING);
            }
            break;
        default:
            break;
    }
    return TRUE;
}

// stop bgm when end to clean the gst
void stop_background_music() {
    if (bgm_pipeline) {
        gst_element_set_state(bgm_pipeline, GST_STATE_NULL);
        gst_object_unref(bgm_pipeline);
        bgm_pipeline = NULL;
        bgm_started = FALSE;
    }
}

// set direction label
char* generateDirectionLabel(enum SnakeDirection direction) {
    switch(direction) {
        case SNAKE_DIRECTION_UP:
            return "North";
            break;
        case SNAKE_DIRECTION_RIGHT:
            return "East";
            break;
        case SNAKE_DIRECTION_DOWN:
            return "South";
            break;
        case SNAKE_DIRECTION_LEFT:
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
void findValueCoordinatesInMatrix(int **matrix, int value, int* coordinates) {
    for (int i = 0; i < game.field_height; i++) {
        for (int j = 0; j < game.field_width; j++) {
            if (matrix[i][j] == value) {
                coordinates[0] = i;
                coordinates[1] = j;
            }
        }
    }
}

// handle keypressed event
gboolean key_pressed(GtkEventControllerKey* self, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    switch (keyval) {
        case GDK_KEY_w:
        case GDK_KEY_Up:
            if (game.snake.direction != SNAKE_DIRECTION_DOWN) {
                game.snake.direction = SNAKE_DIRECTION_UP;
            }
            break;
        case GDK_KEY_s:
        case GDK_KEY_Down:
            if (game.snake.direction != SNAKE_DIRECTION_UP) {
                game.snake.direction = SNAKE_DIRECTION_DOWN;
            }
            break;
        case GDK_KEY_a:
        case GDK_KEY_Left:
            if (game.snake.direction != SNAKE_DIRECTION_RIGHT) {
                game.snake.direction = SNAKE_DIRECTION_LEFT;
            }
            break;
        case GDK_KEY_d:
        case GDK_KEY_Right:
            if (game.snake.direction != SNAKE_DIRECTION_LEFT) {
                game.snake.direction = SNAKE_DIRECTION_RIGHT;
            }
            break;
    }
    return TRUE;
}

// empty game field
void emptyField() {
    for (int i = 0; i < game.field_height; i++) {
        for (int j = 0; j < game.field_width; j++) {
            game.field[i][j] = 0;
            game.snake.position[i][j] = 0;
        }
    }
}

void get_next_snake_coords(int* output, int* currentPosition, enum SnakeDirection direction) {
    switch(direction) {
        case SNAKE_DIRECTION_UP:
            output[0] = currentPosition[0] - 1;
            output[1] = currentPosition[1];
            break;
        case SNAKE_DIRECTION_RIGHT:
            output[0] = currentPosition[0];
            output[1] = currentPosition[1] + 1;
            break;
        case SNAKE_DIRECTION_DOWN:
            output[0] = currentPosition[0] + 1;
            output[1] = currentPosition[1];
            break;
        case SNAKE_DIRECTION_LEFT:
            output[0] = currentPosition[0];
            output[1] = currentPosition[1] - 1;
            break;
    }
}

// refreshes the game field
gboolean refreshField(gpointer user_data) {
    // if the apple was not placed, it is now placed
    while (game.apples_placed != game.apples_target) {
        int appleX;
        int appleY;
        do {
            appleX = rand() % game.field_width;
            appleY = rand() % game.field_height;
        } while (game.snake.position[appleY][appleX] > 0 && game.field[appleY][appleX] != 1);
        game.field[appleY][appleX] = 1;
        game.apples_placed += 1;
    }

    // if the player was not placed, it its now placed
    if (!game.isSnakePlaced) {
        game.snake.position_y = game.field_height / 2;
        game.snake.position_x = game.field_width / 2;
        game.snake.position[game.snake.position_y][game.snake.position_x] = 1;
        game.snake.maxValue = 1;
        game.isSnakePlaced = 1;
        game.snake.direction = SNAKE_DIRECTION_RIGHT;
        game.snake.isGrowing = 0;
    }

    // trigger the play function
    if (!bgm_started) {
        play_background_music();
    }

    if (game.isSnakePlaced) {
        // check field boundaries
        if (game.snake.position_y > 0 && game.snake.direction == SNAKE_DIRECTION_UP) {
            if (game.field[game.snake.position_y - 1][game.snake.position_x] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[game.snake.position_y - 1][game.snake.position_x] > 0) {
                game.isGameOver = 1;
            }
        }
        if (game.snake.position_x > 0 && game.snake.direction == SNAKE_DIRECTION_RIGHT) {
            if (game.field[game.snake.position_y][game.snake.position_x + 1] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[game.snake.position_y][game.snake.position_x + 1] > 0) {
                game.isGameOver = 1;
            }
        }
        if (game.snake.position_y < game.field_height - 1 && game.snake.direction == SNAKE_DIRECTION_DOWN) {
            if (game.field[game.snake.position_y + 1][game.snake.position_x] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[game.snake.position_y + 1][game.snake.position_x] > 0) {
                game.isGameOver = 1;
            }
        }
        if (game.snake.position_x < game.field_width - 1 && game.snake.direction == SNAKE_DIRECTION_LEFT) {
            if (game.field[game.snake.position_y][game.snake.position_x - 1] == 1) {
                game.snake.isGrowing = 1;
            }
            if (game.snake.position[game.snake.position_y][game.snake.position_x - 1] > 0) {
                game.isGameOver = 1;
            }
        }
        if (!game.snake.isGrowing) {
            switch(game.snake.direction) {
                case SNAKE_DIRECTION_UP:
                    if (game.snake.position_y > 0) {
                        game.snake.position[game.snake.position_y - 1][game.snake.position_x] = game.snake.position[game.snake.position_y][game.snake.position_x];
                        game.snake.position_y--;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                case SNAKE_DIRECTION_RIGHT:
                    if (game.snake.position_x < game.field_width - 1) {
                        game.snake.position[game.snake.position_y][game.snake.position_x + 1] = game.snake.position[game.snake.position_y][game.snake.position_x];
                        game.snake.position_x++;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                case SNAKE_DIRECTION_DOWN:
                    if (game.snake.position_y < game.field_height - 1) {
                        game.snake.position[game.snake.position_y + 1][game.snake.position_x] = game.snake.position[game.snake.position_y][game.snake.position_x];
                        game.snake.position_y++;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                case SNAKE_DIRECTION_LEFT:
                    if (game.snake.position_x > 0) {
                        game.snake.position[game.snake.position_y][game.snake.position_x - 1] = game.snake.position[game.snake.position_y][game.snake.position_x];
                        game.snake.position_x--;
                    } else {
                        game.isGameOver = 1;
                    }
                    break;
                }
        }
        for (int i = 0; i < game.field_height; i++) {
            for (int j = 0; j < game.field_width; j++) {
                if (((i != game.snake.position_y || j != game.snake.position_x)) && (game.snake.position[i][j] > 0) && (!game.snake.isGrowing)) {
                    game.snake.position[i][j]--;
                }
            }
        }
        if (game.snake.isGrowing) {
            game.score++;
            int appleCoordinates[2];
            int snakeHead[] = { game.snake.position_y, game.snake.position_x };
            get_next_snake_coords(appleCoordinates, snakeHead, game.snake.direction);
            game.snake.maxValue++;
            game.snake.position[appleCoordinates[0]][appleCoordinates[1]] = game.snake.maxValue;
            game.snake.position_y = appleCoordinates[0];
            game.snake.position_x = appleCoordinates[1];
            game.field[appleCoordinates[0]][appleCoordinates[1]] = 0;
            game.apples_placed -= 1;
            game.snake.isGrowing = 0;
        }
    }

    checkLevelScore();
    updateLabels();
    for (int i = 0; i < game.field_height; i++) {
        for (int j = 0; j < game.field_width; j++) {
            if (game.debug_mode == DEBUG_MODE_PRINT_SNAKE) {
                char st[(game.snake.maxValue / 10) + 1];
                sprintf(st, "%d", game.snake.position[i][j]);
                gtk_label_set_text((GtkLabel*) gameFieldSquare[i][j], st);
            } else if (game.debug_mode == DEBUG_MODE_PRINT_FIELD) {
                char st[3];
                sprintf(st, "%d", game.field[i][j]);
                gtk_label_set_text((GtkLabel*) gameFieldSquare[i][j], st);
            }

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
    gtk_css_provider_load_from_string(provider, ".gameOverExitButton { margin-top: 10px; } .gameOverTitle { font-size: 48px; } .gameFieldSquare { border-radius: 0px; } .gameFieldSquare-even { background-color: green; } .gameFieldSquare-odd { background-color: forestgreen; } .apple { background-color: red; } .snake { background-color: blue; } .snakeHead { border: 3px solid yellow; }");
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
    GtkWidget **gameFieldChildrenBoxes = malloc(sizeof(GtkWidget*) * game.field_height);
    for (int i = 0; i < game.field_height; i++) {
        gameFieldChildrenBoxes[i] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_halign(gameFieldChildrenBoxes[i], GTK_ALIGN_CENTER);
        gtk_widget_set_valign(gameFieldChildrenBoxes[i], GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(gameFieldBox), gameFieldChildrenBoxes[i]);
        for (int j = 0; j < game.field_width; j++) {
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
    gtk_window_present(GTK_WINDOW(window));

    // use timeout to reload field
    g_timeout_add(game.gameFieldRefreshTimeout, refreshField, NULL);

    free(gameFieldChildrenBoxes);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    game.gameFieldRefreshTimeout = 75;
    game.field_width = 25;
    game.field_height = 25;
    game.apples_target = 5;
    game.debug_mode = DEBUG_MODE_NONE;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0)) {
                printf("csnake - https://github.com/Cutotopo/csnake\n===========================================\nOptions:\n  --apples / -a          | Set number of apples to place on the field (default is 5)\n  --debug / -d           | Enable debug flag (one of `print_snake`, `print_field`)\n  --refreshTimeout / -rt | Set game tick interval in milliseconds (default is 75)\n  --size / -s            | Set field size (default is 25x25)\n  --help / -h            | Show this help message\n");
                exit(0);
            }

            if ((strcmp(argv[i], "--refreshTimeout") == 0) || (strcmp(argv[i], "-rt") == 0)) {
                if (atoi(argv[i + 1]) == 0) {
                    printf("Refresh timeout value should be a positive integer.\n");
                    exit(1);
                }
                game.gameFieldRefreshTimeout = atoi(argv[i + 1]);
            }

            if ((strcmp(argv[i], "--debug") == 0) || (strcmp(argv[i], "-d") == 0)) {
                if ((strcmp(argv[i + 1], "print_snake") == 0)) {
                    game.debug_mode = DEBUG_MODE_PRINT_SNAKE;
                } else if ((strcmp(argv[i + 1], "print_field") == 0)) {
                    game.debug_mode = DEBUG_MODE_PRINT_FIELD;
                } else {
                    printf("Invalid debug mode.\n");
                    exit(1);
                }
            }

            if ((strcmp(argv[i], "--apples") == 0) || (strcmp(argv[i], "-a") == 0)) {
                if (atoi(argv[i + 1]) == 0) {
                    printf("Apples target count should be a positive integer.\n");
                    exit(1);
                }
                game.apples_target = atoi(argv[i + 1]);
            }

            if ((strcmp(argv[i], "--size") == 0) || (strcmp(argv[i], "-s") == 0)) {
                if (atoi(argv[i + 1]) == 0 || atoi(argv[i + 2]) == 0) {
                    printf("Both size values should be positive integers.\n");
                    exit(1);
                }
                game.field_height = atoi(argv[i + 1]);
                game.field_width = atoi(argv[i + 2]);
            }
        }
    }

    game.field = malloc(sizeof(int*) * game.field_height);
    game.snake.position = malloc(sizeof(int*) * game.field_height);
    gameFieldSquare = malloc(sizeof(GtkWidget**) * game.field_height);
    for (int i = 0; i < game.field_height; i++) {
        game.field[i] = malloc(sizeof(int) * game.field_width);
        game.snake.position[i] = malloc(sizeof(int) * game.field_width);
        gameFieldSquare[i] = malloc(sizeof(GtkWidget*) * game.field_width);
    }

    // app
    GtkApplication *app;
    gst_init(&argc, &argv);
    int status;

    // app initialization and activation
    app = gtk_application_new("io.github.cutotopo.csnake", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), 0, NULL);

    // quit application
    g_object_unref(app);

    for (int i = 0; i < game.field_height; i++) {
        free(game.snake.position[i]);
        free(game.field[i]);
        free(gameFieldSquare[i]);
    }
    free(game.snake.position);
    free(game.field);
    free(gameFieldSquare);

    return status;
}
