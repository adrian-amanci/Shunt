#ifndef INITIATOR_H
#define INITIATOR_H

#include <systemc>
#include <iostream>
#include "shunt_tlm.h"
#include "shunt_define.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;
using namespace shunt_tlm;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"


// Initiator module generating generic payload transactions

struct Initiator: sc_module
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_initiator_socket<Initiator> socket;
  int m_socket=0;
  int start_sim=0;
  int end_sim=0;

  SC_CTOR(Initiator)
  : socket("socket")  // Construct and name socket
  {
    SC_THREAD(thread_process);
  }

  void thread_process()
  {
    shunt_tlm::cs_tlm_axi3_extension_payload_header_t gp_ext;
    // TLM-2 generic payload transaction, reused across calls to b_transport
    m_socket = shunt_tlm_init_server(MY_PORT);
    tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload;

    cout<<"\nSERVER: shunt_cs_get_cs_header_leader()="<<hex <<shunt_cs_get_cs_header_leader()
        <<" \nshunt_cs_get_tlm_data_leader()="<<hex <<shunt_cs_get_tlm_data_leader()
        <<" \nshunt_cs_get_tlm_axi3_ext_leader()="<<hex<<shunt_cs_get_tlm_axi3_ext_leader()
        <<" \nshunt_cs_get_tlm_axi3_signal_leader()="<<hex<<shunt_cs_get_tlm_axi3_signal_leader()
        <<endl;
    sc_time delay;
    shunt_tlm_command command;
    shunt_long_t tlm_extension_id=0;
    shunt_recv_b_transport(m_socket,*trans,tlm_extension_id,delay);
    command = (shunt_tlm_command)trans->get_command();
    cout<<"SERVER: SHUNT_TLM_START_SIM command="<<hex <<command<<endl;

    delay = sc_time(10, SC_NS);

    // Generate a random sequence of reads and writes
    for (int i = 32; i < 96; i += 4)
    {


      tlm::tlm_command cmd = static_cast<tlm::tlm_command>(rand() % 2);
      if (cmd == tlm::TLM_WRITE_COMMAND) data = 0xFF000000 | i;
      //Initilize axi3 extension
      gp_ext.AxBURST=1;
      gp_ext.AxCACHE=1;
      gp_ext.AxID=10;
      gp_ext.AxLEN=15;
      gp_ext.AxLOCK=0;
      gp_ext.AxPROT=0;
      gp_ext.AxSIZE=3;
      gp_ext.xRESP=1;
      gp_ext.xSTRB=15;
      tlm_extension_id=shunt_cs_get_tlm_axi3_ext_leader();
      //
      // Initialize 8 out of the 10 attributes, byte_enable_length and extensions being unused
      trans->set_command( cmd );
      trans->set_address( i );
      trans->set_data_ptr( reinterpret_cast<unsigned char*>(&data) );
      trans->set_data_length( 4 );
      trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
      trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
      trans->set_dmi_allowed( false ); // Mandatory initial value
      trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
      //
      //socket->b_transport( *trans, delay );  // Blocking transport call
      shunt_send_b_transport(m_socket,*trans,tlm_extension_id, delay );

      shunt_cs_tlm_send_axi3_header(m_socket,&gp_ext);
      shunt_cs_tlm_recv_axi3_header(m_socket,&gp_ext);

      shunt_tlm_print_axi3_header(gp_ext,"SERVER: ");

      shunt_recv_b_transport(m_socket,*trans, tlm_extension_id,delay );
      cout << "SERVER trans recv = { " << (cmd ? 'W' : 'R') << ", " << hex << i
       << " } , data = " << hex << data << " at time " << sc_time_stamp()
       << " delay = " << delay
       << " response = "<< trans->is_response_error()<< endl;
      // Initiator obliged to check response status and delay
      if ( trans->is_response_error() )
        SC_REPORT_ERROR("TLM-2", "Response error from b_transport");

      // Realize the delay annotated onto the transport call
      wait(delay);
    }
    shunt_tlm_send_command(m_socket,SHUNT_TLM_END_SIM);
    shunt_prim_close_socket(m_socket);
    cout <<"LT_simple_sv test is finished"<<endl;
  }

  // Internal data buffer used by initiator with generic payload
  int data;
};

#endif
