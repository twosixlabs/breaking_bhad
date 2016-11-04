#!/usr/bin/python
import sys
import base64
import time

head = """POST /upnp/control/deviceinfo1 HTTP/1.1\r\nContent-Type: text/xml; charset="utf-8"\r\nSOAPACTION: "urn:Belkin:service:deviceinfo:1#GetInformation"\r\nContent-Length: %d\r\nHOST: 192.168.1.143:49153\r\nUser-Agent: CyberGarage-HTTP/1.0\r\n\r\n"""
body="""<?xml version="1.0" encoding="utf-8"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
 <s:Body>
  <u:GetInformation xmlns:u="urn:Belkin:service:deviceinfo:1">
   <UTC>%d</UTC>
   <TimeZone>-04.0</TimeZone>
   <dst>1</dst>
   <DstSupported>1</DstSupported>
  </u:GetInformation>
 </s:Body>
</s:Envelope>
"""

body = body %(int(time.time()))
sys.stdout.write(head%(len(body))+body)
