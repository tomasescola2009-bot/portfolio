# OverlayApp (Windows)

Projeto de exemplo: janela overlay transparente, sempre no topo, "click-through" (não intercepta o rato), que lê coordenadas X/Y de um ficheiro de memória partilhada e desenha um círculo nas coordenadas.


Build (Visual Studio / CMake):

1. Abra a pasta no Visual Studio (File -> Open -> Folder) ou use a linha de comandos com CMake.

2. Para compilar via linha de comandos (Developer x64 Command Prompt):

```bat
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

3. Executáveis gerados:
- `overlay` — a aplicação overlay (ImGui + DirectX11).
- `write_coords` — utilitário para escrever X/Y na memória partilhada (teste).

Uso rápido:

1. Execute `overlay` (ele abrirá uma janela overlay transparente no ecrã).
2. Em outro terminal, execute `write_coords 400 300` para posicionar o círculo.

Notas técnicas:
- O overlay usa uma janela `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE` para ser topmost e click-through.
- As coordenadas são lidas da memória partilhada nomeada `Local\\OverlayCoords` como dois floats (x, y).
- O desenho usa ImGui com o backend DirectX11 (`imgui_impl_dx11`) e Win32 backend (`imgui_impl_win32`). ImGui é baixado automaticamente via CMake `FetchContent`.

Observações:
- Em alguns sistemas, obter perfeita transparência composta pelo DWM pode requerer ajustes adicionais; a implementação usa a standard `example_win32_directx11` approach e deve funcionar com o compositor do Windows moderno.
- Se preferir, posso trocar a renderização para usar uma técnica baseada em `UpdateLayeredWindow` (GDI+/blit) em vez de DirectX11.

Modo de Edição / Hotkey:

- Pressione `INSERT` para alternar entre o modo Overlay (padrão, click-through) e o Modo Menu (interactivo).
- No Modo Menu, pode clicar/arrastar para mover o círculo diretamente; o overlay irá escrever as coordenadas no mapping `Local\\OverlayCoords`.
- No Modo Menu há também um pequeno painel ImGui com controles para ajustar:
  - Raio do círculo (slider 5-100).
  - Cor do círculo (color picker com canal alfa).
  - Botão para fechar a aplicação.

Persistência de Definições:

- Todas as definições (posição X/Y, raio, cor) são guardadas num ficheiro `settings.ini` na pasta de trabalho.
- As definições são carregadas automaticamente ao iniciar; qualquer alteração no Modo Menu é guardada imediatamente.
- Se pretender reinicar com valores por defeito, elimine o ficheiro `settings.ini`.
