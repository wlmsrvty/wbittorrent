echo "Testing ./bittorrent info <torrent>"
dir=$(dirname $0)

echo $#

bittorrent=$1

sample="$dir/sample.torrent"

tmp=$(mktemp -d)

$bittorrent info $sample > "$tmp/got"

cat <<EOF > $tmp/expected
Tracker URL: http://bittorrent-test-tracker.codecrafters.io/announce
Length: 92063
Info Hash: d69f91e6b2ae4c542468d1073a71d4ea13879a7f
Piece Length: 32768
Piece Hashes:
e876f67a2a8886e8f36b136726c30fa29703022d
6e2275e604a0766656736e81ff10b55204ad8d35
f00d937a0213df1982bc8d097227ad9e909acc17
EOF

diff -u $tmp/expected $tmp/got

if [ $? -ne 0 ]; then
    echo "FAILED"
    exit 1
fi