#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios orig_termios;

static void die(const char *msg) {
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
    perror(msg);
    exit(1);
}

static void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        perror("tcsetattr");
}

static void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

static int editorReadKey(void) {
    char c;
    while (1) {
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1) break;
        if (n == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                char seq2;
                if (read(STDIN_FILENO, &seq2, 1) != 1) return '\x1b';
                if (seq2 == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
                return '\x1b';
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    }

    return c;
}

static int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    return 0;
}

static int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

typedef struct erow {
    int size;
    char *chars;
} erow;

struct editorConfig {
    int cx, cy;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    bool dirty;

    char *filename;

    char statusmsg[80];
    time_t statusmsg_time;
} E;

typedef struct abuf {
    char *b;
    size_t len;
} abuf;

static void abInit(abuf *ab) { ab->b = NULL; ab->len = 0; }
static void abAppend(abuf *ab, const char *s, size_t len) {
    char *newb = realloc(ab->b, ab->len + len);
    if (!newb) return;
    memcpy(newb + ab->len, s, len);
    ab->b = newb;
    ab->len += len;
}
static void abFree(abuf *ab) { free(ab->b); }

static void editorUpdateWindowSize(void);

static void editorFreeRow(erow *row) {
    free(row->chars);
}

static void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty = true;
}

static void editorInsertRow(int at, const char *s, int len) {
    if (at < 0 || at > E.numrows) at = E.numrows;
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.numrows++;
    E.dirty = true;
}

static void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->chars[at] = (char)c;
    row->size++;
    E.dirty = true;
}

static void editorRowAppendString(erow *row, const char *s, int len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    E.dirty = true;
}

static void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    E.dirty = true;
}

static void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

static void editorInsertNewline(void) {
    if (E.cy == E.numrows) {
        editorInsertRow(E.cy, "", 0);
        E.cx = 0;
        E.cy++;
        return;
    }

    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    E.cy++;
    E.cx = 0;
    E.dirty = true;
}

static void editorDelChar(void) {
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;

    erow *row = &E.row[E.cy];
    if (E.cx > 0) {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        int prevlen = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
        E.cx = prevlen;
    }
}

static void editorOpen(const char *filename) {
    free(E.filename);
    E.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) return;

    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    while ((n = getline(&line, &cap, fp)) != -1) {
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) n--;
        editorInsertRow(E.numrows, line, (int)n);
    }
    free(line);
    fclose(fp);
    E.dirty = false;
}

static char *editorRowsToString(int *outlen) {
    int totlen = 0;
    for (int j = 0; j < E.numrows; j++) totlen += E.row[j].size + 1;
    if (E.numrows == 0) totlen = 0;

    char *buf = malloc(totlen);
    int p = 0;
    for (int j = 0; j < E.numrows; j++) {
        memcpy(&buf[p], E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        if (j != E.numrows - 1) buf[p++] = '\n';
    }
    *outlen = p;
    return buf;
}

static void editorSave(void) {
    if (!E.filename) return;

    int len;
    char *buf = editorRowsToString(&len);

    int fd = open(E.filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        free(buf);
        snprintf(E.statusmsg, sizeof(E.statusmsg), "Error: %s", strerror(errno));
        E.statusmsg_time = time(NULL);
        return;
    }

    ssize_t written = write(fd, buf, len);
    close(fd);
    free(buf);

    if (written != len) {
        snprintf(E.statusmsg, sizeof(E.statusmsg), "Write Error: %s", strerror(errno));
    } else {
        E.dirty = false;
        snprintf(E.statusmsg, sizeof(E.statusmsg), "%d bytes saved in \"%s\"", (int)written, E.filename);
    }
    E.statusmsg_time = time(NULL);
}

static char *editorPrompt(const char *prompt) {
    size_t bufcap = 128;
    size_t buflen = 0;
    char *buf = malloc(bufcap);
    buf[0] = '\0';

    while (1) {
        snprintf(E.statusmsg, sizeof(E.statusmsg), prompt, buf);
        E.statusmsg_time = time(NULL);

        int c = editorReadKey();
        if (c == '\r') {
            if (buflen != 0) {
                E.statusmsg[0] = '\0';
                return buf;
            }
        } else if (c == '\x1b' || c == CTRL_KEY('g')) {
            E.statusmsg[0] = '\0';
            free(buf);
            return NULL;
        } else if (c == BACKSPACE || c == CTRL_KEY('h') || c == DEL_KEY) {
            if (buflen != 0) {
                buf[--buflen] = '\0';
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen + 1 >= bufcap) {
                bufcap *= 2;
                buf = realloc(buf, bufcap);
            }
            buf[buflen++] = (char)c;
            buf[buflen] = '\0';
        }
    }
}

static void editorScroll(void) {
    if (E.cy < E.numrows) {
        if (E.cx > E.row[E.cy].size) E.cx = E.row[E.cy].size;
    } else {
        E.cx = 0;
    }

    if (E.cy < E.rowoff) E.rowoff = E.cy;
    if (E.cy >= E.rowoff + E.screenrows) E.rowoff = E.cy - E.screenrows + 1;

    if (E.cx < E.coloff) E.coloff = E.cx;
    if (E.cx >= E.coloff + E.screencols) E.coloff = E.cx - E.screencols + 1;
}

static void editorDrawRows(abuf *ab) {
    for (int y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            abAppend(ab, "~", 1);
        } else {
            int len = E.row[filerow].size - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            abAppend(ab, &E.row[filerow].chars[E.coloff], len);
        }

        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

static void editorDrawStatusBar(abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);
    char left[80], right[80];

    int lenleft = snprintf(left, sizeof(left), " %s %s",
                           E.filename ? E.filename : "[no name]",
                           E.dirty ? "(changed)" : "");
    int lenright = snprintf(right, sizeof(right), "%d/%d ",
                            E.cy + 1, E.numrows ? E.numrows : 1);

    if (lenleft > E.screencols) lenleft = E.screencols;

    abAppend(ab, left, lenleft);

    while (lenleft < E.screencols - lenright) {
        abAppend(ab, " ", 1);
        lenleft++;
    }
    if (lenright <= E.screencols) abAppend(ab, right, lenright);

    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

static void editorDrawMessageBar(abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = (int)strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
}

static void editorRefreshScreen(void) {
    editorScroll();

    abuf ab;
    abInit(&ab);

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buf[32];
    int cx = (E.cx - E.coloff) + 1;
    int cy = (E.cy - E.rowoff) + 1;
    if (cx < 1) cx = 1;
    if (cy < 1) cy = 1;
    int n = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy, cx);
    abAppend(&ab, buf, n);

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

static void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                if (E.cy + 1 < E.numrows) {
                    E.cy++;
                    E.cx = 0;
                }
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) E.cy--;
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) E.cy++;
            break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) E.cx = rowlen;
}

static void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

static void editorProcessKeypress(void) {
    static int quit_times = 1;
    int c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0) {
                editorSetStatusMessage("There are unsaved changes - press Ctrl-Q again to exit.");
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
            exit(0);
            break;

        case CTRL_KEY('s'):
            if (!E.filename) {
                char *name = editorPrompt("Save as: %s (ESC to cancel)");
                if (!name) {
                    editorSetStatusMessage("Save cancelled.");
                    break;
                }
                E.filename = name;
            }
            editorSave();
            break;

        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            if (E.cy < E.numrows) E.cx = E.row[E.cy].size;
            break;

        case PAGE_UP:
        case PAGE_DOWN: {
            if (c == PAGE_UP) {
                E.cy = E.rowoff;
            } else if (c == PAGE_DOWN) {
                E.cy = E.rowoff + E.screenrows - 1;
                if (E.cy > E.numrows) E.cy = E.numrows;
            }
            int times = E.screenrows;
            while (times--) editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        } break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;

        case DEL_KEY:
            if (E.cy < E.numrows) {
                editorMoveCursor(ARROW_RIGHT);
                editorDelChar();
            }
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
            editorDelChar();
            break;

        case '\r':
            editorInsertNewline();
            break;

        case '\x1b':
            break;

        default:
            if (!iscntrl(c)) {
                editorInsertChar(c);
            }
            break;
    }

    quit_times = 1;
}

static void editorInit(void) {
    memset(&E, 0, sizeof(E));
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.filename = NULL;
    E.dirty = false;

    int rows, cols;
    if (getWindowSize(&rows, &cols) == -1) die("getWindowSize");
    E.screenrows = rows - 2;
    E.screencols = cols;

    editorSetStatusMessage("HELP: Ctrl-S save | Ctrl-Q exit | ESC/Ctrl-G cancel in dialog");
}

static void editorUpdateWindowSize(void) {
    int rows, cols;
    if (getWindowSize(&rows, &cols) == -1) return;
    E.screenrows = rows - 2;
    E.screencols = cols;
}

int main(int argc, char **argv) {
    enableRawMode();
    editorInit();

    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    while (1) {
        editorUpdateWindowSize();
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
