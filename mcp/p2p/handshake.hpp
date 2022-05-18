#pragma once

#include <mcp/p2p/common.hpp>
#include <mcp/p2p/capability.hpp>
#include <mcp/p2p/host.hpp>
#include <mcp/common/common.hpp>
#include <mcp/common/log.hpp>
#include <mcp/core/config.hpp>

namespace mcp
{
	namespace p2p
	{
		class hankshake_msg
		{
		public:
			hankshake_msg(node_id const & node_id_a, uint16_t const & version_a, mcp::mcp_networks const & network_a, std::list<capability_desc> const & cap_descs_a, mcp::public_key_comp const & enckey_a);
			hankshake_msg(dev::RLP const & r);
			void stream_RLP(dev::RLPStream & s);

			node_id id;
			uint16_t version;
			mcp::mcp_networks network;
			std::list<capability_desc> cap_descs;
			// added by michael at 2/26
			mcp::public_key_comp enckey;
		};

		class host;
		class frame_coder;
		class hankshake : public std::enable_shared_from_this<hankshake>
		{
			friend class frame_coder;
		public:
			hankshake(std::shared_ptr<host> const& _host, std::shared_ptr<bi::tcp::socket> const& _socket) :
				m_host(_host),
				m_remote(0),
				m_originated(false),
				m_socket(_socket),
				m_remoteVersion(0),
				m_transferVersion(0),
				m_idleTimer(m_socket->get_io_service())
			{
				m_nonce = mcp::nonce::get();
			}

			// originated connection.
			hankshake(std::shared_ptr<host> const& _host, std::shared_ptr<bi::tcp::socket> const& _socket, node_id _remote) :
				m_host(_host),
				m_remote(_remote),
				m_originated(true),
				m_socket(_socket),
				m_remoteVersion(0),
				m_transferVersion(0),
				m_idleTimer(m_socket->get_io_service())
			{
				m_nonce = mcp::nonce::get();
			}

			virtual ~hankshake() = default;

			/// Start handshake.
			void start() { transition(); }

			/// Aborts the handshake.
			void cancel();

		private:
			enum State
			{
				Error = -1,
				ExchgPublic,
				AckExchgPublic,
				New,
				AckAuth,
				AckAuthEIP8,
				WriteHello,
				ReadHello,
				StartSession
			};

			///
			void ComposeOutPacket(dev::bytes const& data);

			/// send to socket
			void send(dev::bytes const& data);

			///read header
			void read_info();

			/// read from socket
			void read();

			///get packet size
			uint32_t packet_size();

			///process read packet
			void do_process();

			///write p2p info message to socket, version, public key, transaction to readPublic.
			void writeInfo();

			///reads info message from socket and transitions to writePublic.
			void readInfo();

			/// Write Auth message to socket and transitions to AckAuth.
			void writeAuth();

			/// Reads Auth message from socket and transitions to AckAuth.
			void readAuth();

			/// Write Ack message to socket and transitions to WriteHello.
			void writeAck();

			/// Reads Auth message from socket and transitions to WriteHello.
			void readAck();

			/// Derives ephemeral secret from signature and sets members after Auth has been decrypted.
			void setAuthValues(mcp::signature const& sig, mcp::public_key_comp const& _hePubk, mcp::public_key_comp const& remotePubk, mcp::nonce const& remoteNonce);

			//handshake error 
			void error();

			/// Performs transition for m_nextState.
			virtual void transition(boost::system::error_code _ech = boost::system::error_code());

			/// Timeout for remote to respond to transition events. Enforced by m_idleTimer and refreshed by transition().
			boost::posix_time::milliseconds const c_timeout = boost::posix_time::milliseconds(1800);

			State m_curState = ExchgPublic;
			State m_nextState = ExchgPublic;	//Current or expected state of transition.
			bool m_cancel = false;	//true if error occured

			std::shared_ptr<host> m_host;
			node_id m_remote;	//Public address of remote host.

			dev::bytes m_handshakeOutBuffer;		///< Frame buffer for egress Hello packet.
			dev::bytes m_handshakeInBuffer;		///< Frame buffer for ingress Hello packet.

			bool m_originated = false;	//True if connection is outbound.

			mcp::key_pair_ed m_ecdheLocal = mcp::key_pair_ed::create();
			mcp::nonce m_nonce;

			mcp::public_key_comp m_ecdheRemote;			///< Remote ephemeral public key.
			mcp::nonce m_remoteNonce;				///< nonce generated by remote host for handshake.
			uint64_t m_remoteVersion;

			uint64_t m_transferVersion;				//transfer version, used smaller

			std::unique_ptr<mcp::p2p::frame_coder> m_io;

			std::shared_ptr<bi::tcp::socket> m_socket;

			boost::asio::deadline_timer m_idleTimer;	//< Timer which enforces c_timeout.
            mcp::log m_log = { mcp::log("p2p") };
		};
	}
}