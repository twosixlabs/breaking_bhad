#!/usr/bin/python
import sys
import base64

#if len(sys.argv) < 2:
#	print "Error: please supply file"
#	exit();

header = 'POST /upnp/control/WiFiSetup1 HTTP/1.1\r\nSOAPAction: "urn:Belkin:service:WiFiSetup:1#StopPair"\r\nHost: 192.168.1.12:49153\r\nContent-Type: text/xml\r\nContent-Length: %d\r\n\r\n'
body ="""<?xml version="1.0"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
<SOAP-ENV:Body>
        <m:StopPair xmlns:m="urn:Belkin:service:WiFiSetup:1">

        </m:StopPair>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>"""

sys.stdout.write(header%(len(body))+body)
