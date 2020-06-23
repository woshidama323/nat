const int decompressed_size = xcompress_t::lz4_decompress((const char*)defragment_to_whole_packet.get_body().data(),(char*)new_pakcet.get_body().data(), total_encoded_body_len, max_body_bound_size);
if(decompressed_size > 0)
{
    new_pakcet.get_body().push_back(NULL,decompressed_size); //refill the decompress data
}
else
{
    xwarn_err("xslsocket_t::on_data_packet_recv,decompress fail,decompressed_size=%d vs org_total_body_len=%d",decompressed_size,total_encoded_body_len);
    return enum_xerror_code_bad_packet;
}

xbase-08:22:58.004-T2437:[Error]-(on_fragment_data_packet_recv:2588): xslsocket_t::on_data_packet_recv,decompress fail,decompressed_size=-1384 vs org_total_body_len=1429
从这里看，数据长度是1429，但是decompress出来是负数，说明解压的过程中出问题，包可能不完整，但是我这边暂时不知道怎么判断，怎么样包才算完整？

这里是函数on_fragment_data_packet_recv 收到packet的数据，
(gdb) frame 6
#6  0x0000000001470b72 in top::base::xslsocket_t::on_fragment_data_packet_recv (this=0x7fc26c0c6e90, _linkhead=..., from_xip_addr_low=0, from_xip_addr_high=0, to_xip_addr_low=0, to_xip_addr_high=0, packet=..., cur_thread_id=19, timenow_ms=3430426042, from=0x7fc26c0c6e90)
    at ../../src/xsocket.cpp:2588
2588	../../src/xsocket.cpp: 没有那个文件或目录.
(gdb) p _linkhead
$1 = (_xlink_header &) @0x7fc2e0fe8840: {ver_protocol = 3, flags = 49697, packet_len = 139, extlength = 0 '\000', header_len = 27 '\033', checksum = 2714202200, sequnceid = 5302, to_logic_port = 427, to_logic_port_token = 53663, from_logic_port = 4119,
  from_logic_port_token = 7215, fragment_id = 2, fragments_count = 2, compressrate = 3 '\003'}
(gdb) p packet

$1 = (top::base::xpacket_t &) @0x7fc2e0fe8c80: {from_xip_lowaddr = 0, from_xip_highaddr = 0, to_xip_lowaddr = 0, to_xip_highaddr = 0, from_xip_port = 0, to_xip_port = 0, to_xaddr_token = 0, from_xlink_port = 0, to_xlink_port = 0, _packet_flags = 49697,
  from_ip_port = 9911, to_ip_port = 9911, _MTU = 255 '\377', _TTL = 255 '\377', _process_flags = 772, cookie = 22774630805592, from_ip_addr = "\300\250\062!", to_ip_addr = "0.0.0.0", _body = {<top::base::xmemh_t> = {_front_offset = 283, _back_offset = 395,
      _mem_handle = 0x7fc2e0fe8cf0, _max_capacity = 268435456, _ptr_context = 0x20e8920 <top::base::xcontext_t::instance()::s_global_context>}, local_buf = {140474975166480, 19791209299969, 360287970189639680, 0 <repeats 17 times>}}, _header = {<top::base::xstream_t> = {
    <top::base::xbuffer_t> = {_vptr.xbuffer_t = 0x2006b30 <vtable for top::base::xautostream_t<64>+16>, ptr_context = 0x20e8920 <top::base::xcontext_t::instance()::s_global_context>, block = 0x7fc2e0fe8dc0 "", front_offset = 64, back_offset = 64, capacity_size = 64,
        max_allow_size = 268435455}, m_init_external_mem_ptr = 0x7fc2e0fe8dc0 ""}, local_buf = {0, 0, 0, 0, 0, 0, 0, 0, 0}}}



xbase-08:20:23.689-T2437:[Debug]-(on_fragment_data_packet_recv:2516): xslsocket_t::on_data_packet_recv,defragment 1 piece with sequece_id(5292),fragment_id=1,fragments_count(4),packet size(1429)
xbase-08:20:51.166-T2434:[Info ]-(send_handshake_pdu:2060): xslsocket_t::send_handshake_pdu,total_packet_size(420),detail=xslsocket_t obj(id=4222124673984210,this=140473584202368),local(ip:0.0.0.0,port:9911)-logic(441:34506)-cookie:1493981371 <---> peer(ip:192.168.50.35,port:9911)-logic(0:0)-cookie:0;data_packet[out:0,in:0],keep_alive_packets[out:0,in:0],refcount=3,handle=106,type=33,iostatus=1,sockstatus(2),handshake_status(1)
xbase-08:21:32.497-T2438:[Warn ]-(on_timer_fire:1966): xslsocket_t::on_timer_fire,connection'status is timeout after 311898 ms than expire(5000 ms) after retried count(0);current_time_ms=3430447853 - starttime(3430135955),and close it.detail=xslsocket_t obj(id=4222124673983648,this=140473561273104),local(ip:0.0.0.0,port:9911)-logic(435:6877)-cookie:2972935382 <---> peer(ip:192.168.50.39,port:9911)-logic(0:0)-cookie:0;data_packet[out:0,in:0],keep_alive_packets[out:0,in:0],refcount=3,handle=106,type=33,iostatus=1,sockstatus(2),handshake_status(1)
xbase-08:21:37.344-T2422:[Info ]-(send_handshake_pdu:2060): xslsocket_t::send_handshake_pdu,total_packet_size(714),detail=xslsocket_t obj(id=4222124673984243,this=140474789807552),local(ip:0.0.0.0,port:9911)-logic(442:28822)-cookie:2758622824 <---> peer(ip:192.168.50.46,port:9911)-logic(0:0)-cookie:0;data_packet[out:0,in:0],keep_alive_packets[out:0,in:0],refcount=3,handle=106,type=33,iostatus=1,sockstatus(2),handshake_status(1)
xbase-08:22:20.639-T2438:[Warn ]-(is_alive:1925): xslsocket_t::is_alive failed after idle(143066)ms,idle_timeout_ms=45000,detail=xslsocket_t obj(id=4222124673981752,this=140473013137040),local(ip:0.0.0.0,port:9911)-logic(427:53663)-cookie:2365314685 <---> peer(ip:192.168.50.33,port:9911)-logic(4119:7215)-cookie:2037071509;data_packet[out:76,in:777],keep_alive_packets[out:4,in:1],refcount=9,handle=106,type=33,iostatus=5,sockstatus(4),handshake_status(5)
xbase-08:22:39.500-T2428:[Info ]-(send_handshake_pdu:2060): xslsocket_t::send_handshake_pdu,total_packet_size(844),detail=xslsocket_t obj(id=4222124673984305,this=140473562394384),local(ip:0.0.0.0,port:9911)-logic(443:17112)-cookie:2931781657 <---> peer(ip:192.168.50.39,port:9911)-logic(0:0)-cookie:0;data_packet[out:0,in:0],keep_alive_packets[out:0,in:0],refcount=3,handle=106,type=33,iostatus=1,sockstatus(2),handshake_status(1)
xbase-08:22:57.964-T2438:[Info ]-(send_handshake_pdu:2060): xslsocket_t::send_handshake_pdu,total_packet_size(567),detail=xslsocket_t obj(id=4222124673981752,this=140473013137040),local(ip:0.0.0.0,port:9911)-logic(427:53663)-cookie:2365314685 <---> peer(ip:192.168.50.33,port:9911)-logic(4119:7215)-cookie:2037071509;data_packet[out:76,in:780],keep_alive_packets[out:4,in:1],refcount=10,handle=106,type=33,iostatus=5,sockstatus(4),handshake_status(5)
xbase-08:22:57.964-T2432:[Info ]-(send_handshake_pdu:2060): xslsocket_t::send_handshake_pdu,total_packet_size(624),detail=xslsocket_t obj(id=4222124673984345,this=140473015518720),local(ip:0.0.0.0,port:9911)-logic(444:51675)-cookie:4020281322 <---> peer(ip:192.168.50.42,port:9911)-logic(0:0)-cookie:0;data_packet[out:0,in:0],keep_alive_packets[out:0,in:0],refcount=3,handle=106,type=33,iostatus=1,sockstatus(2),handshake_status(1)
xbase-08:22:57.969-T2437:[Debug]-(on_fragment_data_packet_recv:2516): xslsocket_t::on_data_packet_recv,defragment 1 piece with sequece_id(5302),fragment_id=1,fragments_count(2),packet size(1429)
xbase-08:22:58.004-T2438:[Info ]-(send_handshake_pdu:2060): xslsocket_t::send_handshake_pdu,total_packet_size(295),detail=xslsocket_t obj(id=4222124673981752,this=140473013137040),local(ip:0.0.0.0,port:9911)-logic(427:53663)-cookie:2365314685 <---> peer(ip:192.168.50.33,port:9911)-logic(4119:7215)-cookie:2037071509;data_packet[out:76,in:784],keep_alive_packets[out:4,in:1],refcount=12,handle=106,type=33,iostatus=6,sockstatus(8),handshake_status(5)
xbase-08:22:58.004-T2438:[Warn ]-(is_alive:1925): xslsocket_t::is_alive failed after idle(143066)ms,idle_timeout_ms=45000,detail=xslsocket_t obj(id=4222124673981755,this=140473897563072),local(ip:0.0.0.0,port:9911)-logic(428:24393)-cookie:1663016050 <---> peer(ip:192.168.50.41,port:9911)-logic(4117:59398)-cookie:1605406352;data_packet[out:115,in:597],keep_alive_packets[out:4,in:1],refcount=7,handle=106,type=33,iostatus=5,sockstatus(4),handshake_status(5)
xbase-08:22:58.004-T2437:[Error]-(on_fragment_data_packet_recv:2588): xslsocket_t::on_data_packet_recv,decompress fail,decompressed_size=-1384 vs org_total_body_len=1429