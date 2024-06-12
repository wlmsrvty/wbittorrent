<p align="center">
  <img width="200" height="200" src="https://github.com/wlmsrvty/ownbittorrent/assets/73073487/b220acb8-1c7e-4982-bb4c-9df44b69bcd6">
</p>
<p align="center"><i>small logo created with Midjourney</i></p>

# wBittorrent
![GitHub Actions Workflow Status](https://github.com/wlmsrvty/wbittorrent/actions/workflows/build.yml/badge.svg)


A (very) basic BitTorrent client CLI written in C++. It implements from scratch parts of the [BitTorrent protocol](https://www.bittorrent.org/beps/bep_0003.html).

For now, it only supports downloading a single file from a single peer. It does not support seeding and it uses a naive strategy for piece selection (downloading pieces in order).

What it currently does:

- bencode
- .torrent file parsing
- peers discovery via tracker (HTTP)
- peer handshake and communication (TCP) for downloading pieces

## Usage

```sh
./bittorrent info <torrent file>
./bittorrent peers <torrent file>
./bittorrent download_piece -o <output_file> <torrent file>
./bittorrent download -o <output_file> <torrent file>
```

## Build

```bash
# building
cmake . -B build
cmake --build build

# building with tests and debug
cmake . -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target test
```

## Resources used

- [BitTorrent protocol specification](https://www.bittorrent.org/beps/bep_0003.html)
- [theory.org wiki BitTorrent page](https://wiki.theory.org/BitTorrentSpecification)
- [CodeCrafters BitTorrent challenge](https://app.codecrafters.io/courses/bittorrent/overview)

