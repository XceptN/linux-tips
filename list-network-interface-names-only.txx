If you need to get a name-only list of network interfaces on the command line, use below:

ip -o link show | sed 's/[[:digit:]]+: \([[:alnum:]]+\):.*/\1/'
