# Test script will test each LED colour, and sound of a netlight
#
# Requires built netlight command
#
# If no IP address or name is passed in, will use avahi-browse to find the device

NAME=$1

if [[ -z "$NAME" ]] ; then
  NAME=$(avahi-browse --terminate --resolve _nl5._tcp | grep hostname | egrep -o "netlight.*local")
fi

for i in $(seq 0 4) ; do
  ./netlight  -u $i -c 000000 $NAME
  sleep 0.1
done
for i in $(seq 0 4) ; do
  ./netlight  -u $i -c ffffff $NAME
  sleep 0.1
done
for i in $(seq 0 4) ; do
  ./netlight  -u $i -c ff0000 $NAME
  sleep 0.1
done
for i in $(seq 0 4) ; do
  ./netlight  -u $i -c 00ff00 $NAME
  sleep 0.1
done
for i in $(seq 0 4) ; do
  ./netlight  -u $i -c 0000ff -s chirp $NAME
  sleep 1
done