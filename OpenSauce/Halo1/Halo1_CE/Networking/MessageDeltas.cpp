/*
	Yelo: Open Sauce SDK
		Halo 1 (CE) Edition

	See license\OpenSauce\Halo1_CE for specific license information
*/
#include "Common/Precompile.hpp"
#include "Networking/MessageDeltas.hpp"

#include "Game/EngineFunctions.hpp"
#include "Memory/MemoryInterface.hpp"
#include "Networking/MessageDefinitions.hpp"
#include "Networking/Networking.hpp"

namespace Yelo
{
	namespace MessageDeltas
	{
#define __EL_INCLUDE_ID			__EL_INCLUDE_NETWORKING
#define __EL_INCLUDE_FILE_ID	__EL_NETWORKING_MESSAGE_DELTAS
#include "Memory/_EngineLayout.inl"

		s_misc_encoding_globals* MiscEncodingGlobals()			PTR_IMP_GET2(misc_encoding_globals);
		Enums::message_delta_encoding_class* EncodingClass()	PTR_IMP_GET2(g_message_delta_encoding_class);
		byte* PacketBufferReceived()							PTR_IMP_GET2(mdp_packet_buffer_received_data);
		byte* PacketBufferSent()								PTR_IMP_GET2(mdp_packet_buffer_sent_data);

		const message_delta_definition* const* OriginalPackets() { return GET_DPTR2(message_delta_packets); }

#ifndef YELO_NO_NETWORK
		void PLATFORM_API NetworkGameClientHandleMessageDeltaMessageBodyEx(); // forward declare

		static message_delta_definition* NewMessageDeltaList[Enums::k_message_deltas_new_count + 1]; // +1 so we can compile when there are no new deltas
		const message_delta_definition* const* NewPackets() { return NewMessageDeltaList; }

		void InitializeNewMessageDeltaList()
		{
			message_delta_definition** org_mdptrs = CAST_QUAL(message_delta_definition**, MessageDeltas::OriginalPackets());

			// copy the original halopc delta definitions
			size_t x = 0;
			for(; x < Enums::k_message_deltas_count; x++)
				NewMessageDeltaList[x] = org_mdptrs[x];

			// copy the new yelo delta definitions
			for(size_t i = 0; i < Enums::k_message_deltas_yelo_count; i++, x++)
				NewMessageDeltaList[x] = kYeloMessageDeltas[i];
		}
		// Initialize the engine's code to reference our NewMessageDeltaList
		void InitializeEngineWithNewMessageDeltaList()
		{
			size_t x = 0;
			for(x = 0; x < NUMBEROF(MessageDeltaPointerReferences); x++)
				*MessageDeltaPointerReferences[x] = NewMessageDeltaList;

			for(x = 0; x < NUMBEROF(MessageDeltaPointerEndChecks); x++)
				*MessageDeltaPointerEndChecks[x] = &NewMessageDeltaList[Enums::k_message_deltas_new_count];

			for(x = 0; x < NUMBEROF(MessageDeltaTypeCountChecks8bit); x++)
				*MessageDeltaTypeCountChecks8bit[x] = Enums::k_message_deltas_new_count;
		}
		void InitializeYeloMessageDeltaDefinitions()
		{
			InitializeNewMessageDeltaList();
			InitializeEngineWithNewMessageDeltaList();

			// need a method to hook network_game_server_handle_message_delta_message so we can intercept client based packets
			Memory::WriteRelativeCall(&NetworkGameClientHandleMessageDeltaMessageBodyEx, GET_FUNC_VPTR(NETWORK_GAME_CLIENT_HANDLE_MESSAGE_DELTA_MESSAGE_BODY_CALL));
		}
#endif

		void Initialize()
		{
#ifndef YELO_NO_NETWORK

			if(Enums::k_message_deltas_yelo_count > 0)
				InitializeYeloMessageDeltaDefinitions();
#endif
		}

		void Dispose()
		{
		}


		static void EnableMessageDeltaSteroids()
		{
			// By forcing the encoding class to lan, we shouldn't have to worry about doing anything here.
			// However, once a new game starts, the code may re-initialize the encoding class. 
			// If we want permanent changes to the message delta encoding properties, we should do them here.
		}
		bool EnableNetworkingSteroids()
		{
			static bool g_enabled = false;

			// Always force the game to think it's running on a LAN
			*EncodingClass() = Enums::_message_delta_encoding_class_lan;

			if(!g_enabled)
			{
				Networking::network_update_globals* settings = Networking::UpdateSettings();
				settings->remote_player.action_baseline_update_rate = 30;
				settings->remote_player.position_update_rate = 15;
				settings->remote_player.position_baseline_update_rate = 15;

				EnableMessageDeltaSteroids();

				g_enabled = true;
			}

			return g_enabled;
		}



#ifndef YELO_NO_NETWORK

#pragma region Yelo packet handlers
		static bool NetworkGameClientHandleMessageDeltaMessageBodyOverride(Networking::s_network_game_client* client, message_dependant_header* header)
		{
			//int32 msg_type = header->decoding_information->definition_type;

			return false;
		}
		// Returns true if we handled a Yelo packet
		static bool NetworkGameClientHandleMessageDeltaMessageBodyYelo(Networking::s_network_game_client* client, message_dependant_header* header)
		{
			int32 msg_type = header->decoding_information->definition_type;

			if(msg_type >= Enums::k_message_deltas_count && 
				msg_type < Enums::k_message_deltas_new_count)
			{
				const MessageDeltas::packet_decoder& decoder = kYeloMessageDeltaDecoders[msg_type - Enums::k_message_deltas_count];

				// Validate that the packet can be decoded in our current network state
				if( TEST_FLAG(decoder.Flags, Enums::_message_deltas_new_client_bit) == Networking::IsClient() || 
					TEST_FLAG(decoder.Flags, Enums::_message_deltas_new_server_bit) == Networking::IsServer() )
					decoder.Proc(client, header);

				return true;
			}

			return false;
		}

		// Hooks the network_game_client_handle_message_delta_message_body function to allow us to intercept our own packets
		API_FUNC_NAKED static void PLATFORM_API NetworkGameClientHandleMessageDeltaMessageBodyEx()
		{
			static uint32 TEMP_ASM_ADDR = GET_DATA_PTR(DONT_SEND_OBJECT_NEW_MSG);

			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(NETWORK_GAME_CLIENT_HANDLE_MESSAGE_DELTA_MESSAGE_BODY);

			// I'm not taking any chances with our network code doing some nasty stuff to the 
			// registers so, we store them temporarily
			static message_dependant_header* local_store_header;
			static Networking::s_network_game_client* local_store_client;

			__asm
			{
				// eax = message_dependant_header*
				// esi = client data

				mov		byte ptr [TEMP_ASM_ADDR], TRUE
				mov		local_store_header, eax
				mov		local_store_client, ecx

				push	eax
				push	ecx
				call	NetworkGameClientHandleMessageDeltaMessageBodyYelo
				test	al, al
				jnz		the_end	// if not zero then we handled a Yelo message and not a stock game message

				mov		eax, local_store_header
				mov		ecx, local_store_client
				push	eax
				push	ecx
				call	NetworkGameClientHandleMessageDeltaMessageBodyOverride
				test	al, al
				jnz		the_end	// if not zero then we handled a stock message ourselves

				mov		ecx, local_store_client
				mov		eax, local_store_header
				call	TEMP_CALL_ADDR

the_end:
				retn
			}
		}
#pragma endregion


#pragma region SvWrite
		int32 SvWrite(int32 data_size_in_bits, int32 dont_bit_encode, int32 dont_flush)
		{
			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(NETWORK_GAME_SERVER_WRITE);

			byte header_buffer = 1;
			int32 shit = dont_bit_encode ? 3 : 2;

			__asm {
				push	shit
				push	dont_flush
				push	dont_bit_encode
				push	1
				lea		edi, header_buffer
				push	edi
				push	GET_PTR2(mdp_packet_buffer_sent_data)
				mov		edi, GET_PTR2(global_network_game_server_data)
				mov		edi, [edi] // get the game server's s_network_connection
				mov		ebx, data_size_in_bits
				call	TEMP_CALL_ADDR
				add		esp, 4 * 6
			}
		}
#pragma endregion

#pragma region EncodeStateless
		int32 EncodeStateless(long_enum mode, long_enum def_type, void** headers, void** datas, void** baselines, int32 iterations, int32 unk)
		{
			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(MDP_ENCODE_STATELESS);

			__asm {
				mov		edx, 0x7FF8
				mov		eax, GET_PTR2(mdp_packet_buffer_sent_data)
				push	unk
				push	iterations
				push	baselines
				push	datas
				push	headers
				push	def_type
				push	mode
				call	TEMP_CALL_ADDR
				add		esp, 4 * 7
			}
		}
#pragma endregion

#pragma region DecodeStatelessIterated
		bool DecodeStatelessIterated(void* header, void* out_buffer)
		{
			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(MDP_DECODE_STATELESS_ITERATED);

			__asm {
				mov		eax, header
				mov		ecx, out_buffer
				call	TEMP_CALL_ADDR
			}
		}
#pragma endregion

#pragma region ClientSendMessageToServer
		bool ClientSendMessageToServer(int32 data_size_in_bits)
		{
			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(NETWORK_CONNECTION_FLUSH_QUEUE);
			static uint32 TEMP_CALL_ADDR2 = GET_FUNC_PTR(BITSTREAM_WRITE_BUFFER);

			bool result = false;

			if(data_size_in_bits == 0) return result;

			byte shit = 1;

			__asm {
				//push	edi
				push	ebx // if this is uncommented, the compiler catches it, but if it is commented, it fucking doesn't....

				mov		ebx, data_size_in_bits
				inc		ebx

				mov		eax, GET_PTR2(global_network_game_client_data)
				mov		edi, [eax+0xADC]			// s_network_game_client->connection
				test	byte ptr [edi+0xA8C], 1		// test for _connection_create_server_bit
				jnz		_the_exit

				//////////////////////////////////////////////////////////////////////////
				// bitstream_get_bits_remaining
				mov		ecx, [edi+0x1C]
				mov		edx, [edi+0x24]
				mov		eax, [edi+0x20]
				lea		esi, [edi+0x10]
				shl		ecx, 3
				sub		edx, ecx
				sub		edx, eax
				inc		edx
				//////////////////////////////////////////////////////////////////////////
				cmp		ebx, edx
				jle		bit_writes
				push	1
				push	edi
				call	TEMP_CALL_ADDR // NETWORK_CONNECTION_FLUSH_QUEUE
				add		esp, 4 * 2
				test	al, al
				jz		_the_exit

bit_writes:
				add		[edi+0xA80], ebx
				push	1
				lea		ecx, shit
				mov		eax, esi
				call	TEMP_CALL_ADDR2 // BITSTREAM_WRITE_BUFFER
				add		esp, 4 * 1

				push	data_size_in_bits
				mov		ecx, GET_PTR2(mdp_packet_buffer_sent_data)
				mov		eax, esi
				mov		byte ptr [esi+0x1C], 0
				call	TEMP_CALL_ADDR2
				add		esp, 4 * 1
				mov		byte ptr [esi+0x1C], 0

				mov		result, TRUE
_the_exit:
				pop		ebx
				//pop		edi
			}

			return result;
		}
#pragma endregion

#pragma region SvSendMessageToAll
		bool SvSendMessageToAll(int32 data_size_in_bits, int32 dont_bit_encode, int32 dont_flush, int32 send_even_after_fail, int32 shit)
		{
			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(NETWORK_GAME_SERVER_SEND_MESSAGE_TO_ALL_MACHINES); // all machines in-game

			if(shit == -1)
				shit = dont_bit_encode ? 3 : 2;

			__asm {
				push	shit
				push	send_even_after_fail // false = if fail, don't send
				push	dont_flush
				push	dont_bit_encode
				push	GET_PTR2(mdp_packet_buffer_sent_data)
				push	1 // new packet system, 0 for old packet system
				mov		ecx, GET_PTR2(global_network_game_server_data)
				mov		eax, data_size_in_bits
				call	TEMP_CALL_ADDR
				add		esp, 4 * 6
			}
		}
#pragma endregion

#pragma region SvSendMessageToMachine
		bool SvSendMessageToMachine(int32 machine_index, int32 data_size_in_bits, int32 dont_bit_encode, int32 dont_flush, int32 send_even_after_fail, int32 shit)
		{
			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(NETWORK_GAME_SERVER_SEND_MESSAGE_TO_MACHINE);

			if(shit == -1)
				shit = dont_bit_encode ? 3 : 2;

			__asm {
				push	esi

				push	shit
				push	send_even_after_fail // false = if fail, don't send
				push	dont_flush
				push	dont_bit_encode
				push	data_size_in_bits
				push	GET_PTR2(mdp_packet_buffer_sent_data)
				push	1
				mov		eax, machine_index
				mov		esi, GET_PTR2(global_network_game_server_data)
				call	TEMP_CALL_ADDR
				add		esp, 4 * 7

				pop		esi
			}
		}
#pragma endregion

#pragma region DiscardIterationBody
		void DiscardIterationBody(void* header)
		{
			static uint32 TEMP_CALL_ADDR = GET_FUNC_PTR(MDP_DISCARD_ITERATION_BODY);

			__asm {
				mov		eax, header
				call	TEMP_CALL_ADDR
			}
		}
#pragma endregion
#endif
	};
};