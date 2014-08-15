//
// Copyright (C) 2011-2014 Codership Oy <info@codership.com>
//


#ifndef GALERA_IST_HPP
#define GALERA_IST_HPP

#include <pthread.h>

#include "wsrep_api.h"
#include "galera_gcs.hpp"
#include "trx_handle.hpp"
#include "gu_config.hpp"
#include "gu_lock.hpp"
#include "gu_monitor.hpp"

#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "asio.hpp"
#ifdef HAVE_ASIO_SSL_HPP
#include "asio/ssl.hpp"
#endif
#include <stack>
#include <set>

namespace gcache
{
    class GCache;
}

namespace galera
{
    class TrxHandle;

    namespace ist
    {
        void register_params(gu::Config& conf);

        class Receiver
        {
        public:
            static std::string const RECV_ADDR;

            Receiver(gu::Config& conf, TrxHandle::SlavePool&, const char* addr);
            ~Receiver();

            std::string   prepare(wsrep_seqno_t, wsrep_seqno_t, int);
            void          ready();
            int           recv(TrxHandle** trx);
            wsrep_seqno_t finished();
            void          run();

        private:

            void interrupt();

            std::string                                   recv_addr_;
            asio::io_service                              io_service_;
            asio::ip::tcp::acceptor                       acceptor_;
#ifdef HAVE_ASIO_SSL_HPP
            asio::ssl::context                            ssl_ctx_;
#endif
            gu::Mutex                                     mutex_;
            gu::Cond                                      cond_;

            class Consumer
            {
            public:

                Consumer() : cond_(), trx_(0) { }
                ~Consumer() { }

                gu::Cond&  cond()              { return cond_; }
                void       trx(TrxHandle* trx) { trx_ = trx;   }
                TrxHandle* trx() const         { return trx_;  }

            private:

                gu::Cond   cond_;
                TrxHandle* trx_;
            };

            std::stack<Consumer*> consumers_;
            wsrep_seqno_t         current_seqno_;
            wsrep_seqno_t         last_seqno_;
            gu::Config&           conf_;
            TrxHandle::SlavePool& trx_pool_;
            pthread_t             thread_;
            int                   error_code_;
            int                   version_;
#ifdef HAVE_ASIO_SSL_HPP
            bool                  use_ssl_;
#endif
            bool                  running_;
            bool                  ready_;
        };

        class Sender
        {
        public:

            Sender(const gu::Config& conf,
                   gcache::GCache& gcache,
                   const std::string& peer,
                   int version);
            ~Sender();

            void send(wsrep_seqno_t first, wsrep_seqno_t last);

            void cancel()
            {
#ifdef HAVE_ASIO_SSL_HPP
                if (use_ssl_ == true)
                {
                    ssl_stream_.lowest_layer().close();
                }
                else
#endif
                {
                    socket_.close();
                }
            }

        private:

            asio::io_service                         io_service_;
            asio::ip::tcp::socket                    socket_;
#ifdef HAVE_ASIO_SSL_HPP
            asio::ssl::context                       ssl_ctx_;
            asio::ssl::stream<asio::ip::tcp::socket> ssl_stream_;
#endif
            const gu::Config&                        conf_;
            gcache::GCache&                          gcache_;
            int                                      version_;
            bool                                     use_ssl_;

            Sender(const Sender&);
            void operator=(const Sender&);
        };


        class AsyncSender;
        class AsyncSenderMap
        {
        public:
            AsyncSenderMap(GCS_IMPL& gcs, gcache::GCache& gcache)
                :
                senders_(),
                monitor_(),
                gcs_(gcs),
                gcache_(gcache) { }
            void run(const gu::Config& conf,
                     const std::string& peer,
                     wsrep_seqno_t,
                     wsrep_seqno_t,
                     int);
            void remove(AsyncSender*, wsrep_seqno_t);
            void cancel();
            gcache::GCache& gcache() { return gcache_; }
        private:
            std::set<AsyncSender*> senders_;
            // use monitor instead of mutex, it provides cancellation point
            gu::Monitor            monitor_;
            GCS_IMPL&              gcs_;
            gcache::GCache&        gcache_;
        };


    } // namespace ist
} // namespace galera

#endif // GALERA_IST_HPP
