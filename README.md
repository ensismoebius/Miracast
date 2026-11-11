# miracast

Cliente e receptor **Miracast (Wi-Fi Display)** em linha de comando para Linux.

- **`cast`** envia a tela e o áudio do computador para uma TV ou outro receptor Miracast.
- **`sink`** recebe um fluxo Miracast e o exibe no computador.
- **`scan`** lista os receptores por perto.

Dois transportes são suportados:

| Transporte | Precisa de | Quando usar |
|---|---|---|
| **Miracast sobre a rede (MS-MICE)** | só uma rede IP comum (Wi-Fi ou cabo); o receptor precisa implementar MS-MICE | o caminho mais simples: não exige hardware Wi-Fi especial |
| **Wi-Fi Direct (P2P)** | adaptador cujo driver tenha os modos `P2P-client`/`P2P-GO`, `wpa_supplicant` com P2P e root | o Miracast "clássico", para TVs sem MS-MICE |

O `cast` tenta a rede primeiro (descoberta automática por mDNS) e cai para o Wi-Fi Direct se não achar o receptor.

> **Estado do projeto.** O modo pela rede foi testado de ponta a ponta com uma LG webOS (50NANO75SPA): tela, áudio e descoberta automática. O Wi-Fi Direct está implementado, mas **não foi testado contra hardware real**, porque o adaptador de desenvolvimento não tem os modos P2P. A implementação segue as especificações públicas e **não foi certificada pela Wi-Fi Alliance**.

## Recursos

- Sessão RTSP do Wi-Fi Display completa: mensagens M1 a M7, keep-alive, pedido de IDR e encerramento.
- Vídeo H.264 (CBP/CHP) em MPEG-TS sobre RTP; negocia a maior resolução e taxa de quadros suportadas pelos dois lados, de 640x480p60 até 1920x1080p60.
- Áudio do sistema em AAC-LC 48 kHz estéreo, quando o receptor suporta.
- Captura de tela no Wayland pela API `org.gnome.Mutter.ScreenCast` (niri, GNOME) com PipeWire, sem diálogo de portal; no X11 usa `ximagesrc`.
- Encoder, bitrate e buffers configuráveis, inclusive encoder por GPU (`vah264enc`).
- Descoberta automática de receptores por mDNS (`_display._tcp`).

## Requisitos

- Linux, compilador C++17 e CMake 3.16 ou mais novo
- GStreamer (testado com 1.28): plugins *base*, *good*, *bad*, *ugly* e *libav*
- `gst-plugin-va` (GPU e importação de quadros DMA-BUF, necessário em compositores como o niri)
- PipeWire (captura de tela e de áudio)
- Só para Wi-Fi Direct: `wpa_supplicant` com P2P e Wi-Fi Display, `iproute2`, `dnsmasq` e um cliente DHCP (`dhcpcd`, `dhclient` ou `udhcpc`)

Instalação das dependências:

```sh
# Arch Linux
sudo pacman -S --needed base-devel cmake pkgconf gstreamer gst-plugins-base \
    gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav gst-plugin-va \
    gst-plugin-pipewire pipewire dnsmasq

# Debian / Ubuntu
sudo apt install build-essential cmake pkg-config libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev libglib2.0-dev gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav gstreamer1.0-pipewire dnsmasq iproute2
```

## Compilação

```sh
git clone <url-do-repositório> miracast
cd miracast
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure   # testes unitários e de integração em memória
```

O binário fica em `build/miracast`.

## Instalação

```sh
sudo cmake --install build                        # /usr/local
sudo cmake --install build --prefix /usr          # ou outro prefixo
```

Instala o binário em `bin/`, a man page em `share/man/man1/`, a licença e este README em `share/doc/`, e a unit do systemd `miracast-sink.service` em `lib/systemd/system/`. Para desinstalar, apague os arquivos listados em `build/install_manifest.txt`.

Depois de instalar, leia a documentação completa com `man miracast`.

## Uso

### Enviar a tela para a TV (rede)

```sh
miracast scan --no-p2p        # lista os receptores na rede
miracast cast                 # usa o único receptor encontrado
miracast cast minha-tv        # escolhe pelo nome (mostrado no scan)
miracast cast 192.168.0.3     # ou pelo endereço IP, sem descoberta
```

A TV pode pedir confirmação na tela na primeira conexão. Encerre com `Ctrl+C`.

Exemplo com ajustes de qualidade e latência:

```sh
miracast cast minha-tv --output eDP-1 --encoder vah264enc \
    --max-width 1280 --max-fps 30 --bitrate 6000 --buffer-ms 60
```

### Enviar por Wi-Fi Direct

Precisa de root e de um adaptador com P2P:

```sh
sudo miracast scan -i wlan0
sudo miracast cast "Nome da TV" --p2p -i wlan0
```

Se o adaptador não suportar P2P, o programa avisa e sugere o modo pela rede. Confira com `iw phy | grep -A8 "interface modes"` e procure por `P2P-client` e `P2P-GO`.

### Receber um fluxo (`sink`)

```sh
miracast sink --no-p2p --listen 0.0.0.0     # pela rede local
sudo miracast sink -i wlan0                 # por Wi-Fi Direct (vira dono do grupo)
```

Também há a unit `miracast-sink.service` para deixar o receptor rodando como serviço (`sudo systemctl enable --now miracast-sink`).

### Principais opções

| Opção | Descrição |
|---|---|
| `--output NOME` | monitor a compartilhar (ex.: `eDP-1`) |
| `--encoder NOME` | `x264enc` (CPU, padrão), `vah264enc` (GPU) ou outro encoder H.264 do GStreamer |
| `--encoder-opts "k=v ..."` | propriedades extras do encoder |
| `--bitrate KBPS` | bitrate do vídeo (padrão 8000) |
| `--max-width PX`, `--max-fps N` | limites de resolução e quadros (padrão 1920 e 60) |
| `--buffer-ms N` | fila entre captura e encoder; `0` desliga (padrão 100) |
| `--jitter-ms N` | (sink) buffer de recepção (padrão 40) |
| `--no-audio`, `--audio-source F` | desliga o áudio / usa outra fonte de áudio |
| `--capture F`, `--test-pattern` | captura personalizada / padrão de teste |
| `--mice`, `--p2p`, `--no-p2p`, `--host IP` | escolha manual do transporte |
| `-v`, `-vv` | mais logs / rastro do protocolo |

A lista completa está em `miracast --help` e em `man miracast`.

### Códigos de saída

Segue o `sysexits.h`: `0` sucesso, `64` uso incorreto, `68` receptor não encontrado, `69` serviço indisponível (sem P2P, conexão recusada), `70` erro de protocolo, `71` erro do sistema, `75` falha temporária.

## Solução de problemas

| Sintoma | Causa provável |
|---|---|
| `this interface cannot do Wi-Fi Direct` | o driver não expõe modos P2P; use o modo pela rede ou outro adaptador |
| `no element "udpsrc"` / `"avenc_aac"` | faltam `gst-plugins-good` / `gst-libav` |
| `stream error: no more input formats` na captura | o compositor só entrega DMA-BUF; instale `gst-plugin-va` |
| a TV não aparece no `scan` | o receptor pode não suportar MS-MICE (sem a porta TCP 7250); tente o Wi-Fi Direct |
| conecta mas não chega imagem | o firewall deste computador precisa permitir a porta TCP 7236 de entrada |
| imagem com atraso | reduza `--buffer-ms`, `--bitrate` e a resolução; use `--encoder vah264enc` |

## Como funciona

```
                 RTSP (M1..M7) ─────────────────────────────┐
   cast ── MS-MICE (TCP 7250) / Wi-Fi Direct ──► receptor     │
   │                                                           │
   └─ tela (PipeWire) ─► H.264 ─┐                              │
      áudio (PipeWire) ─► AAC ──┴─► MPEG-TS ─► RTP/UDP ───────┘
```

- `src/rtsp_peer`, `sink_session`, `source_session`: máquinas de estado do protocolo, independentes de rede e de mídia (por isso testáveis em memória).
- `src/mice_protocol`, `mdns_discovery`, `dns_packet`: Miracast sobre a rede e descoberta.
- `src/p2p_link`, `wpa_ctrl`, `group_network`: Wi-Fi Direct via `wpa_supplicant`.
- `src/gst_source_media`, `gst_sink_media`, `screen_capture`, `encoder_chain`: mídia com GStreamer e captura de tela.

## Desenvolvimento

O código segue o estilo do kernel Linux (`.clang-format`, tabs de 8 colunas), tem cabeçalho SPDX em todos os arquivos, um tipo por arquivo e documentação em cada função. Para formatar:

```sh
clang-format -i src/*.cpp src/*.hpp tests/*
```

## Limitações conhecidas

- Sem criptografia (HDCP, DTLS) nem PIN do MS-MICE.
- Áudio só na direção fonte → receptor, em AAC-LC estéreo.
- Wi-Fi Direct sem teste em hardware real.

## Referências

- Wi-Fi Display Technical Specification v2.1 (Wi-Fi Alliance)
- [MS-MICE]: Miracast over Infrastructure Connection Establishment Protocol (Microsoft)

## Licença

GPL-2.0-or-later. Veja o arquivo [LICENSE](LICENSE).
