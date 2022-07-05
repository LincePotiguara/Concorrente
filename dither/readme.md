# Requisitos
Antes de compilar instale o kit de desenvolvimento x11, `sudo apt install libx11-dev`, então compile o arquivo main.c com o comando 
gcc opening.c -g -lX11 -lpthread -lm -o run
# Execução
Para executar o programa use
`./run`

Isso irá mostrar um teste comum gradiente.

Passar uma imagem é opcional:
`./run img.jpg`

habilita teste do gradiente 500x500 e 2 threads
`./run 500 2`

Habilita concorrência numa imagem para a impressão da imagem em tela
`gcc opening.c -g -lX11 -lpthread -lm -o run -DCONCURRENCY=1 && ./run imagem.jpg`