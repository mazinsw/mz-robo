// Colisão com os cantos da tela
#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>
#include <time.h>
#include <fmod/fmod.h>
#define BLOCO 0
#define ESPACO 1
#define ROBO 2
#define ALVO 3
#define MONSTRO 4
#define QUANTIDADE 20
#define PASSO 10

typedef struct
{
    char **matriz;
    int lagura;
    int altura;
    int rx;
    int ry;
    int ax;
    int ay;
} Mapa;

typedef struct
{
    int x;
    int y;
} PONTO;

typedef struct
{
    PONTO p;
    int espera;
} Monstro;

void    InitFMOD ( )
{

    FSOUND_SetOutput ( FSOUND_OUTPUT_DSOUND );      // DirectSound
    FSOUND_SetDriver ( 0 );                         // Sound Driver (default 0)
    FSOUND_SetMixer ( FSOUND_MIXER_AUTODETECT );    // Mixer
    FSOUND_Init ( 44100, 32, 0 );                   // 44.1 kHz and 32 channels
}

int play(const char * musica)
{
    FSOUND_SAMPLE * son = NULL;                     // NULL is significant
    // Loading of our sound (2D)
    son = FSOUND_Sample_Load ( FSOUND_FREE, musica, FSOUND_HW2D, 0, 0);
    return FSOUND_PlaySoundEx ( FSOUND_FREE, son, NULL, FALSE );
}

int le_mapa(Mapa * mp)
{
    FILE * fp = fopen("mapa.txt", "r");
    if(!fp)
        return 0;
    int i, j;

    fscanf(fp, "%d %d%*c", &mp->altura, &mp->lagura);
    mp->matriz = (char**)malloc(sizeof(char*)*mp->altura);
    for(i = 0; i < mp->altura; i++)
    {
        mp->matriz[i] = (char*)malloc(sizeof(char) * mp->lagura);

        for(j = 0; j < mp->lagura; j++)
        {
            fscanf(fp, "%c", &mp->matriz[i][j]);

            if(mp->matriz[i][j] == 'A')
            {
                mp->ax = j;
                mp->ay = i;
            }
            else if(mp->matriz[i][j] == 'R')
            {
                mp->rx = j;
                mp->ry = i;
                mp->matriz[i][j] = ' ';//está aqui
            }
        }
        fscanf(fp, "%*c");//descarta o \n
    }
    return 1;
}

void libera_mapa(Mapa * mp)
{
    int i;
    for(i = 0; i < mp->altura; i++)
        free(mp->matriz[i]);
    free(mp->matriz);
}

int pode_ir(Mapa *mp, int x, int y)
{
    if(x < 0 || x >= mp->lagura || y < 0 || y >= mp->altura)
        return 0;
    if(mp->matriz[y][x] == '#')
        return 0;
    return 1;
}

int monstro_pode_ir(Mapa *mp, int x, int y)
{
    if(x < 0 || x >= mp->lagura || y < 0 || y >= mp->altura)
        return 0;
    if(mp->matriz[y][x] != ' ')
        return 0;
    return 1;
}

void ponto_vazio(Mapa * mp, PONTO * p)
{
    int x, y;
    srand(time(NULL));
    do
    {
        x = rand() % mp->lagura;
        y = rand() % mp->altura;
        if(monstro_pode_ir(mp, x, y))
            break;
    }
    while(1);
    p->x = x;
    p->y = y;
    mp->matriz[y][x] = 'M';
}

void anda_aleatorio(Mapa * mp, PONTO * p)
{
    int lado;

    //srand(time(NULL));
    lado = rand() % 4;
    switch(lado)
    {
    case 0:
        if(monstro_pode_ir(mp, p->x + 1, p->y))
        {
            mp->matriz[p->y][p->x] = ' ';
            p->x++;
            mp->matriz[p->y][p->x] = 'M';
            return;
        }
        break;
    case 1:
        if(monstro_pode_ir(mp, p->x, p->y + 1))
        {
            mp->matriz[p->y][p->x] = ' ';
            p->y++;
            mp->matriz[p->y][p->x] = 'M';
            return;
        }
        break;
    case 2:
        if(monstro_pode_ir(mp, p->x - 1, p->y))
        {
            mp->matriz[p->y][p->x] = ' ';
            p->x--;
            mp->matriz[p->y][p->x] = 'M';
            return;
        }
        break;
    case 3:
        if(monstro_pode_ir(mp, p->x, p->y - 1))
        {
            mp->matriz[p->y][p->x] = ' ';
            p->y--;
            mp->matriz[p->y][p->x] = 'M';
            return;
        }
        break;
    }
}

int main()
{
    Mapa mapa;
    int i, j;
    int espera = 0;
    int perdeu = 0;
    int ganhou = 0;

    BITMAP * desenhos[5];
    BITMAP * choro;
    Monstro monstros[QUANTIDADE];

    allegro_init();
    set_color_depth(32);
    install_keyboard();
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0);

    choro = load_bitmap("choro.bmp", NULL);
    if(!choro)
        return 1;

    desenhos[BLOCO] = load_bitmap("wall blue.bmp", NULL);
    if(!desenhos[BLOCO])
        return 1;

    desenhos[ESPACO] = load_bitmap("piso.bmp", NULL);
    if(!desenhos[ESPACO])
        return 1;

    desenhos[ROBO] = load_bitmap("robo.bmp", NULL);
    if(!desenhos[ROBO])
        return 1;

    desenhos[ALVO] = load_bitmap("alvo.bmp", NULL);
    if(!desenhos[ALVO])
        return 1;

    desenhos[MONSTRO] = load_bitmap("monstro.bmp", NULL);
    if(!desenhos[MONSTRO])
        return 1;

    if(!le_mapa(&mapa))
        return 1;

    //insere aleatóriamente os monstros
    for(i = 0; i < QUANTIDADE; i++)
    {
        ponto_vazio(&mapa, &monstros[i].p);
        monstros[i].espera = 0;
    }

    // Criando BUFFER, ela é um ponteiro para um BITMAP, pode ter qualquer nome
    BITMAP *buffer = NULL;
    buffer = create_bitmap(640, 480);
    InitFMOD ( );       // Initialization and loading sound


    int channel = play("music.mp3");
    // Laço principal
    while( !key[KEY_ESC] && !perdeu && !ganhou)
    {
        if ( key[KEY_UP] && pode_ir(&mapa, mapa.rx, mapa.ry - 1))
        {
            if(espera > PASSO)
            {
                mapa.ry--;
                espera = 0;
            }
            espera++;
        }

        if ( key[KEY_DOWN] && pode_ir(&mapa, mapa.rx, mapa.ry + 1))
        {
            if(espera > PASSO)
            {
                mapa.ry++;
                espera = 0;
            }
            espera++;
        }

        if ( key[KEY_LEFT] && pode_ir(&mapa, mapa.rx - 1, mapa.ry))
        {
            if(espera > PASSO)
            {
                mapa.rx--;
                espera = 0;
            }
            espera++;
        }

        if ( key[KEY_RIGHT] && pode_ir(&mapa, mapa.rx + 1, mapa.ry))
        {
            if(espera > PASSO)
            {
                mapa.rx++;
                espera = 0;
            }
            espera++;
        }

        // limpa o nosso novo buffer
        clear( buffer );

        //movimenta aleatóriamente os monstros
        for(i = 0; i < QUANTIDADE; i++)
        {
            if(monstros[i].espera > 10 + 9 * rand() % 10)
            {
                anda_aleatorio(&mapa, &monstros[i].p);
                monstros[i].espera = 0;
            }
            monstros[i].espera++;
        }

        for(i = 0; i < mapa.altura; i++)
        {
            for(j = 0; j < mapa.lagura; j++)
            {
                switch(mapa.matriz[i][j])
                {
                case '#'://bloco
                    masked_blit(desenhos[BLOCO], buffer, 0, 0,
                                32 + j * 24, 24 + i * 24,
                                desenhos[BLOCO]->w, desenhos[BLOCO]->h);
                    break;
                default://espaço em branco
                    masked_blit(desenhos[ESPACO], buffer, 0, 0,
                                32 + j * 24, 24 + i * 24,
                                desenhos[ESPACO]->w, desenhos[ESPACO]->h);
                }
            }
        }

        // imprime o buffer na tela
        masked_blit(desenhos[ROBO], buffer, 0, 0,
                    32 + mapa.rx * 24, 24 + mapa.ry * 24,
                    desenhos[ROBO]->w, desenhos[ROBO]->h);

        masked_blit(desenhos[ALVO], buffer, 0, 0,
                    32 + mapa.ax * 24, 24 + mapa.ay * 24,
                    desenhos[ALVO]->w, desenhos[ALVO]->h);

        for(i = 0; i < QUANTIDADE; i++)
        {
            masked_blit(desenhos[MONSTRO], buffer, 0, 0,
                        32 + monstros[i].p.x * 24, 24 + monstros[i].p.y * 24,
                        desenhos[MONSTRO]->w, desenhos[MONSTRO]->h);
            if(mapa.rx == monstros[i].p.x && mapa.ry == monstros[i].p.y)
                perdeu = 1;
            if(mapa.rx == mapa.ax && mapa.ry == mapa.ay)
                ganhou = 1;
        }
        if(perdeu)
        {
            stretch_blit(choro, buffer, 0, 0, choro->w, choro->h,
                         0, 0, SCREEN_W, SCREEN_H);
            textprintf_ex( buffer, font, SCREEN_W / 2 - 100, SCREEN_H / 2 - 10, makecol(255, 20, 20), -1, "Que pena voce perdeu!");
            FSOUND_StopSound(channel);
            channel = play("ahh.mp3");
        }
        if(ganhou)
            textprintf_ex( buffer, font, 32, 10, makecol(20, 255, 20), -1, "Voce ganhou!");
        blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

        // essa função faz o allegro esperar um pouco antes de voltar para o while
        rest(10);
    }
    while( !key[KEY_ESC] )
        rest(10);
    FSOUND_Close ( );   // Close sound
    destroy_bitmap(desenhos[BLOCO]);
    destroy_bitmap(desenhos[ESPACO]);
    destroy_bitmap(desenhos[ROBO]);
    destroy_bitmap(desenhos[ALVO]);
    destroy_bitmap(desenhos[MONSTRO]);
    libera_mapa(&mapa);
    allegro_exit();
    return 0;
}
END_OF_MAIN();
