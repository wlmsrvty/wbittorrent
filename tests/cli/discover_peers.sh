echo "Testing ./bittorrent peers <torrent>"
dir=$(dirname $0)

echo $#

bittorrent=$1

sample="$dir/sample.torrent"

tmp=$(mktemp -d)

$bittorrent peers $sample | grep -E "(([0-9]{1,3}\.){3})([0-9]{1,3}):[0-9]{1,5}"

if [ $? -ne 0 ]; then
    echo "FAILED"
    exit 1
fi