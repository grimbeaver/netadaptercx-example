# NetAdapterCx Example

This example was created to demonstrate a bug I am running into with NetAdapterCx.

I started a discussion on the OSR NTDEV community for this issue:

https://community.osr.com/t/netadaptercx-ipv4-interface-failed-to-bind/58791

This driver works just fine when the `MediaType` in the INF is set to `NdisMediumIP`.  However when set to `NdisMedium802_3` it will fail to bind to both TCPIP and TCPIPV6 for reasons unknown.

The development environment is:
 * Visual Studio 2022 (17.10.0)
 * SDK 10.0.22621.3233
 * WDK 10.0.22621.2428
  
The target PC is:
 * Windows 11 Pro 23H2
 * OS build	22631.3593
 * Windows Feature Experience Pack 1000.22700.1003.0

Problem has also been observed on fully updated Window 10 Pro.

## Setup
Edit the netadaptercx-example.inf and change the hardware ID to a PCI device in your system that you can load the driver against.  All PCI resource configuration is skipped so in theory it should work with any device.

Change `MediaType` in the INF by adjusting which line is commented out and rebuild to try the two configurations.

```
; Using NdisMedium802_3 the adapter will fail to bind to both TCPIP and TCPIPV6
*MediaType              = 0     ; NdisMedium802_3
; Using NdisMediumIP the adapter binds just fine and functions correctly
;*MediaType              = 19    ; NdisMediumIP
```

## Event Viewer Messages
```xml
- <Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event">
- <System>
  <Provider Name="Tcpip" /> 
  <EventID Qualifiers="49152">4207</EventID> 
  <Version>0</Version> 
  <Level>2</Level> 
  <Task>0</Task> 
  <Opcode>0</Opcode> 
  <Keywords>0x80000000000000</Keywords> 
  <TimeCreated SystemTime="2024-06-09T04:10:45.6208539Z" /> 
  <EventRecordID>1506</EventRecordID> 
  <Correlation /> 
  <Execution ProcessID="4" ThreadID="11924" /> 
  <Channel>System</Channel> 
  <Computer>lime</Computer> 
  <Security /> 
  </System>
- <EventData>
  <Data /> 
  <Data>IPv4</Data> 
  <Data>28</Data> 
  <Binary>0000000003003000000000006F1000C006000000BB0000C000000000000000000000000000000000</Binary> 
  </EventData>
  </Event>
```
```xml
- <Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event">
- <System>
  <Provider Name="Tcpip" /> 
  <EventID Qualifiers="49152">4207</EventID> 
  <Version>0</Version> 
  <Level>2</Level> 
  <Task>0</Task> 
  <Opcode>0</Opcode> 
  <Keywords>0x80000000000000</Keywords> 
  <TimeCreated SystemTime="2024-06-09T04:10:45.6208539Z" /> 
  <EventRecordID>1507</EventRecordID> 
  <Correlation /> 
  <Execution ProcessID="4" ThreadID="11924" /> 
  <Channel>System</Channel> 
  <Computer>lime</Computer> 
  <Security /> 
  </System>
- <EventData>
  <Data /> 
  <Data>IPv6</Data> 
  <Data>28</Data> 
  <Binary>0000000003003000000000006F1000C006000000BB0000C000000000000000000000000000000000</Binary> 
  </EventData>
  </Event>
```