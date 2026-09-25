#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../inttype.h"
#include "../vm/forth_vm.h"

// just a "good enough" solution so the linux terminal has the same behavior as the windows one

#define ASKF_ASCII_DEL 127
#define ASKF_ASCII_ESC 27 

#define ASKF_ANSI_ESCAPE        "\x1b["

#define ASKF_ANSI_DISABLE_WRAP "\x1b[?7l"
#define ASKF_ANSI_ENABLE_WRAP  "\x1b[?7h"

#define ASKF_ANSI_ERASE_IN_LINE "K"
#define ASKF_ANSI_ERASE_CURSOR_TILL_END  "J"
#define ASKF_ANSI_MOVE_CURSOR_UP    "A"
#define ASKF_ANSI_MOVE_CURSOR_DOWN  "B"
#define ASKF_ANSI_MOVE_CURSOR_RIGHT "C"
#define ASKF_ANSI_MOVE_CURSOR_LEFT  "D"

#define ASKF_ANSI_MOVE_CURSOR_UP_CHAR    'A'
#define ASKF_ANSI_MOVE_CURSOR_DOWN_CHAR  'B'
#define ASKF_ANSI_MOVE_CURSOR_RIGHT_CHAR 'C'
#define ASKF_ANSI_MOVE_CURSOR_LEFT_CHAR  'D'

typedef struct {
    u64             cursor;
    u64              index;
    u64           capacity;

    u64     rendered_lines;
    u64  offset_from_start;
    ascii      base[KB(4)];
} AskForth_TerminalBuffer;

typedef struct { 
    struct termios original;
} AskForth_Terminal;

static AskForth_Terminal          askf_linux_terminal;

static AskForth_TerminalBuffer askf_linux_terminalbuf = {
    .index             = 0,
    .cursor            = 0,
    .rendered_lines    = 0,
    .offset_from_start = 0,
    .capacity          = KB(4)
};

// set linux terminal to non canonical mode  and read input one at time

static void askf_terminal_begin( void ) {
    tcgetattr( STDIN_FILENO, &askf_linux_terminal.original );

    struct termios t = askf_linux_terminal.original;

    t.c_lflag &= ~(ICANON);
    t.c_lflag &= ~(ECHO);

    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;

    tcsetattr( STDIN_FILENO, TCSANOW, &t );
    //printf( ASKF_ANSI_DISABLE_WRAP );
    printf( "\r" );
    fflush( stdout );
}

static u64 askf_terminal_width( void ) {
    struct winsize w;
    if ( ioctl( STDOUT_FILENO, TIOCGWINSZ, &w ) != 0 )
        return 80;

    return w.ws_col;
}

static void askf_redraw_buffer( AskForth_TerminalBuffer* buf ) {
    u64 term_width = askf_terminal_width();

    u64 cursor_line = buf->cursor / term_width;
    u64 cursor_col  = buf->cursor % term_width;

    u64 old_lines   = buf->rendered_lines;
    u64 new_lines   = buf->index / term_width;

    if ( new_lines > old_lines ) {
        for ( u64 x = 0; x < (new_lines - old_lines); x++ ) 
            printf("\n");
    } 

    u64 last_line    = 0;

    if ( buf->index > term_width )
        last_line    = ( buf->index - 1 ) / term_width;

    if (  cursor_line > 0 ) {
        printf( ASKF_ANSI_ESCAPE "%llu" ASKF_ANSI_MOVE_CURSOR_UP, cursor_line );
    } 
    if ( old_lines > new_lines )  {
        printf( ASKF_ANSI_ESCAPE "%llu" ASKF_ANSI_MOVE_CURSOR_UP, old_lines - new_lines );
    }
    
    printf( "\r" );

    printf( ASKF_ANSI_ESCAPE ASKF_ANSI_ERASE_CURSOR_TILL_END );


    u64 lines_read = 0;
    for ( u64 c = 0; c < buf->index; c++) {
        if ( c != 0 && c % term_width == 0 ) {
            printf("\r\n");
        }

        putchar( buf->base[c] );
    }

    // if ( old_lines > new_lines )  {
    //     printf( ASKF_ANSI_ESCAPE "%lluM", old_lines - new_lines );
    // }

    if ( last_line > cursor_line )
        printf(ASKF_ANSI_ESCAPE "%llu" ASKF_ANSI_MOVE_CURSOR_UP, last_line - cursor_line );

    printf( "\r" );

    if ( cursor_col > 0 ) {
        printf( ASKF_ANSI_ESCAPE "%llu" ASKF_ANSI_MOVE_CURSOR_RIGHT, cursor_col );
    }  
    else if ( cursor_col == term_width && cursor_line != 0 )  {
        printf( ASKF_ANSI_ESCAPE  ASKF_ANSI_MOVE_CURSOR_UP );
    }
    else if ( cursor_col == 0 && cursor_line != 0 )  {
        printf( ASKF_ANSI_ESCAPE  ASKF_ANSI_MOVE_CURSOR_DOWN );
    } 

    buf->rendered_lines = new_lines;

    fflush( stdout );
}

// set linux terminal back to it's original state

static void askf_terminal_end( void ) { 
    printf( ASKF_ANSI_ENABLE_WRAP );

    tcsetattr( STDIN_FILENO, TCSANOW, &askf_linux_terminal.original );
    fflush( stdout );
}

// functions for input buffer manipulation

// shift bytes 1 byte to right starting from the index offset to cursor offset, leaving base[cursor_idx] open
static boolean _shift_right_buffer( AskForth_TerminalBuffer* buf ){
    if ( buf->index + 1 > buf->capacity-1 )
        return FALSE;

    for ( u64 x = buf->index; x > buf->cursor; x-- ) {
        buf->base[x] = buf->base[x-1];
    }

    return TRUE;
}

// shift bytes 1 byte to left starting from the cursor offset to index offset, removing base[cursor_idx] value
static boolean _shift_left_buffer( AskForth_TerminalBuffer* buf ) {
    if ( buf->cursor == 0 || buf->index == 0 )
        return FALSE;

    for ( u64 x = buf->cursor; x < buf->index; x++ ) {
        buf->base[x-1] = buf->base[x];
    }
    return TRUE;
}

static void askf_terminal_add_char( AskForth_TerminalBuffer* buf, ascii _char ) {

    if ( buf->cursor >= buf->index && buf->index < buf->capacity-1 ) {
        buf->base[buf->cursor++] = _char;
    } else if ( buf->cursor < buf->index ) {
        if ( !_shift_right_buffer( buf ) )
            return;
        buf->base[buf->cursor++] = _char;
    }
    buf->index++;
}

static void askf_terminal_remove_char( AskForth_TerminalBuffer* buf, ascii _char ) { 
    if ( buf->cursor == 0 || buf->index == 0 )
        return;

    if ( buf->cursor >= buf->index ) {
        buf->base[buf->cursor] = _char;
    } else if ( buf->cursor < buf->index ) {
        if ( !_shift_left_buffer( buf ) )
            return;
    }
    buf->cursor--;
    buf->index--;
}

static void askf_move_cursor_left( AskForth_TerminalBuffer* buf ) {
    if ( buf->cursor == 0 || buf->index == 0 )
        return;

    buf->cursor--;
}

static void askf_move_cursor_right( AskForth_TerminalBuffer* buf ) {
    if ( buf->cursor >= buf->index ) 
        return;

    buf->cursor++;
}

// main function
static void askf_read_linux( void ) {
    askf_terminal_begin();

    askf_linux_terminalbuf.index  = 0;
    askf_linux_terminalbuf.cursor = 0;

    char c;
    ascii sequence[2];

    while ( TRUE ) {
        read( STDIN_FILENO, &c, 1);

        switch ( c ) {
            case '\r':
            case '\n':
                askf_linux_terminalbuf.base[askf_linux_terminalbuf.index++] = '\n';
                goto end_while;
            case ASKF_ASCII_DEL:
                askf_terminal_remove_char( &askf_linux_terminalbuf, (ascii)c );
                break;
            case ASKF_ASCII_ESC:
                read( STDIN_FILENO, (char*)&sequence[0], 1);
                read( STDIN_FILENO, (char*)&sequence[1], 1);

                if ( sequence[0] == '[' && sequence[1] == ASKF_ANSI_MOVE_CURSOR_LEFT_CHAR )
                    askf_move_cursor_left( &askf_linux_terminalbuf );
                else if ( sequence[0] == '[' &&  sequence[1] == ASKF_ANSI_MOVE_CURSOR_RIGHT_CHAR )
                    askf_move_cursor_right( &askf_linux_terminalbuf );

                break;
            default:
                askf_terminal_add_char( &askf_linux_terminalbuf, (ascii)c );
                break;
        }

        askf_redraw_buffer( &askf_linux_terminalbuf );
    }
    end_while:
    askf_terminal_end();

    printf(" ");

    AskForthVm* vm = askf_get_global_vm();

    COPY( askf_linux_terminalbuf.base, vm->input_buffer->base, askf_linux_terminalbuf.index );
    vm->input_buffer->index = askf_linux_terminalbuf.index;
    vm->istack->sources[0].in_max = vm->input_buffer->index;
    vm->input_buffer->base[vm->input_buffer->index] = '\0';
}

