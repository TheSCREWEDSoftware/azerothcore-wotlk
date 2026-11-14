/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LoginRESTService.h"
#include "BigNumber.h"
#include "Config.h"
#include "CryptoHash.h"
#include "DatabaseEnv.h"
#include "IoContext.h"
#include "Log.h"
#include "ProtobufJSON.h"
#include "QueryResult.h"
#include "Realm.h"
#include "Resolver.h"
#include "SessionManager.h"
#include "SslContext.h"
#include "SteadyTimer.h"
#include "Util.h"
#include "httpget.h"
#include "httppost.h"
#include "soapH.h"

int ns1__executeCommand(soap*, char*, char**) { return SOAP_OK; }

int32 handle_get_plugin(soap* soapClient)
{
    return sLoginService.HandleGet(soapClient);
}

int32 handle_post_plugin(soap* soapClient)
{
    return sLoginService.HandlePost(soapClient);
}

bool LoginRESTService::Start(Acore::Asio::IoContext& ioContext)
{
    _bindIP = sConfigMgr->GetOption<std::string>("BindIP", "0.0.0.0");
    _port = sConfigMgr->GetOption<int32>("LoginREST.Port", 8081);
    if (_port < 0 || _port > 0xFFFF)
    {
        LOG_ERROR("server.rest", "Specified login service port ({}) out of allowed range (1-65535), defaulting to 8081", _port);
        _port = 8081;
    }

    Acore::Asio::Resolver resolver(ioContext);

    std::string configuredAddress = sConfigMgr->GetOption<std::string>("LoginREST.ExternalAddress", "127.0.0.1");
    Optional<boost::asio::ip::tcp::endpoint> externalAddress = resolver.Resolve(boost::asio::ip::tcp::v4(), configuredAddress, std::to_string(_port));
    if (!externalAddress)
    {
        LOG_ERROR("server.rest", "Could not resolve LoginREST.ExternalAddress {}", configuredAddress);
        return false;
    }

    _externalAddress = *externalAddress;

    configuredAddress = sConfigMgr->GetOption<std::string>("LoginREST.LocalAddress", "127.0.0.1");
    Optional<boost::asio::ip::tcp::endpoint> localAddress = resolver.Resolve(boost::asio::ip::tcp::v4(), configuredAddress, std::to_string(_port));
    if (!localAddress)
    {
        LOG_ERROR("server.rest", "Could not resolve LoginREST.LocalAddress {}", configuredAddress);
        return false;
    }

    _localAddress = *localAddress;

    // set up form inputs
    Battlenet::JSON::Login::FormInput* input;
    _formInputs.set_type(Battlenet::JSON::Login::LOGIN_FORM);
    input = _formInputs.add_inputs();
    input->set_input_id("account_name");
    input->set_type("text");
    input->set_label("E-mail");
    input->set_max_length(320);

    input = _formInputs.add_inputs();
    input->set_input_id("password");
    input->set_type("password");
    input->set_label("Password");
    input->set_max_length(16);

    input = _formInputs.add_inputs();
    input->set_input_id("log_in_submit");
    input->set_type("submit");
    input->set_label("Log In");

    _loginTicketCleanupTimer = std::make_unique<boost::asio::steady_timer>(ioContext);
    _loginTicketCleanupTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(10));
    _loginTicketCleanupTimer->async_wait(std::bind(&LoginRESTService::CleanupLoginTickets, this, std::placeholders::_1));

    _thread = std::thread(std::bind(&LoginRESTService::Run, this));
    return true;
}

void LoginRESTService::Stop()
{
    _stopped = true;
    _loginTicketCleanupTimer->cancel();
    _thread.join();
}

boost::asio::ip::tcp::endpoint const& LoginRESTService::GetAddressForClient(boost::asio::ip::address const& address) const
{
    if (address.is_loopback())
        return _localAddress;
    else if (_localAddress.address().is_loopback())
        return _externalAddress;

    if (!address.is_v4() || !_localAddress.address().is_v4())
        return _externalAddress;

    // classful netmask of the local address (boost dropped address_v4::netmask)
    uint32 local = _localAddress.address().to_v4().to_uint();
    uint32 netmask = (local >> 31) == 0 ? 0xFF000000 : ((local >> 30) == 2 ? 0xFFFF0000 : 0xFFFFFF00);
    if ((netmask & address.to_v4().to_uint()) == (netmask & local))
        return _localAddress;

    return _externalAddress;
}

void LoginRESTService::Run()
{
    soap soapServer(SOAP_C_UTFSTRING, SOAP_C_UTFSTRING);

    // check every 3 seconds if world ended
    soapServer.accept_timeout = 3;
    soapServer.recv_timeout = 5;
    soapServer.send_timeout = 5;
    soapServer.bind_flags = SO_REUSEADDR; // restart without waiting for TIME_WAIT sockets
    if (!soap_valid_socket(soap_bind(&soapServer, _bindIP.c_str(), _port, 100)))
    {
        LOG_ERROR("server.rest", "Couldn't bind to {}:{}", _bindIP.c_str(), _port);
        return;
    }

    LOG_INFO("server.rest", "Login service bound to http://{}:{}", _bindIP.c_str(), _port);

    http_post_handlers handlers[] =
    {
        { "application/json;charset=utf-8", handle_post_plugin },
        { "application/json", handle_post_plugin },
        { nullptr, nullptr }
    };

    soap_register_plugin_arg(&soapServer, &http_get, (void*)&handle_get_plugin);
    // gsoap 2.8.1xx dispatches GET requests to fget instead of reporting SOAP_GET_METHOD to the httpget plugin
    soapServer.fget = &handle_get_plugin;
    soap_register_plugin_arg(&soapServer, &http_post, handlers);
    soap_register_plugin_arg(&soapServer, &ContentTypePlugin::Init, (void*)"application/json;charset=utf-8");

    // Use our already ready ssl context
    soapServer.ctx = Battlenet::SslContext::instance().native_handle();
    soapServer.ssl_flags = SOAP_SSL_RSA;

    while (!_stopped)
    {
        if (!soap_valid_socket(soap_accept(&soapServer)))
            continue;   // ran into an accept timeout

        std::shared_ptr<soap> soapClient = std::make_shared<soap>(soapServer);
        boost::asio::ip::address_v4 address(soapClient->ip);
        if (soap_ssl_accept(soapClient.get()) != SOAP_OK)
        {
            LOG_DEBUG("server.rest", "Failed SSL handshake from IP={}", address.to_string().c_str());
            continue;
        }

        LOG_DEBUG("server.rest", "Accepted connection from IP={}", address.to_string().c_str());

        std::thread([soapClient]
        {
            soap_serve(soapClient.get());
        }).detach();
    }

    // and release the context handle here - soap does not own it so it should not free it on exit
    soapServer.ctx = nullptr;

    LOG_INFO("server.rest", "Login service exiting...");
}

int32 LoginRESTService::HandleGet(soap* soapClient)
{
    boost::asio::ip::address_v4 address(soapClient->ip);
    std::string ip_address = address.to_string();

    LOG_DEBUG("server.rest", "[{}:{}] Handling GET request path=\"{}\"", ip_address.c_str(), soapClient->port, soapClient->path);

    static std::string const expectedPath = "/bnetserver/login/";
    if (strstr(soapClient->path, expectedPath.c_str()) != &soapClient->path[0])
        return 404;

    return SendResponse(soapClient, _formInputs);
}

int32 LoginRESTService::HandlePost(soap* soapClient)
{
    boost::asio::ip::address_v4 address(soapClient->ip);
    std::string ip_address = address.to_string();

    LOG_DEBUG("server.rest", "[{}:{}] Handling POST request path=\"{}\"", ip_address.c_str(), soapClient->port, soapClient->path);

    static std::string const expectedPath = "/bnetserver/login/";
    if (strstr(soapClient->path, expectedPath.c_str()) != &soapClient->path[0])
        return 404;

    char *buf;
    size_t len;
    soap_http_body(soapClient, &buf, &len);

    Battlenet::JSON::Login::LoginForm loginForm;
    Battlenet::JSON::Login::LoginResult loginResult;
    if (!JSON::Deserialize(buf, &loginForm))
    {
        if (soap_register_plugin_arg(soapClient, &ResponseCodePlugin::Init, nullptr) != SOAP_OK)
            return 500;

        ResponseCodePlugin* responseCode = reinterpret_cast<ResponseCodePlugin*>(soap_lookup_plugin(soapClient, ResponseCodePlugin::PluginId));
        ASSERT(responseCode);

        responseCode->ErrorCode = 400;

        loginResult.set_authentication_state(Battlenet::JSON::Login::LOGIN);
        loginResult.set_error_code("UNABLE_TO_DECODE");
        loginResult.set_error_message("There was an internal error while connecting to Battle.net. Please try again later.");
        return SendResponse(soapClient, loginResult);
    }

    std::string login;
    std::string password;

    for (int32 i = 0; i < loginForm.inputs_size(); ++i)
    {
        if (loginForm.inputs(i).input_id() == "account_name")
            login = loginForm.inputs(i).value();
        else if (loginForm.inputs(i).input_id() == "password")
            password = loginForm.inputs(i).value();
    }

    Utf8ToUpperOnlyLatin(login);
    Utf8ToUpperOnlyLatin(password);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_INFO);
    stmt->SetData(0, login);
    stmt->SetData(1, CalculateShaPassHash(login, std::move(password)));
    if (PreparedQueryResult result = LoginDatabase.Query(stmt))
    {
        std::unique_ptr<Battlenet::Session::AccountInfo> accountInfo = std::make_unique<Battlenet::Session::AccountInfo>();
        accountInfo->LoadResult(result);

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_CHARACTER_COUNTS_BY_BNET_ID);
        stmt->SetData(0, accountInfo->Id);
        if (PreparedQueryResult characterCountsResult = LoginDatabase.Query(stmt))
        {
            do
            {
                Field* fields = characterCountsResult->Fetch();
                accountInfo->GameAccounts[fields[0].Get<uint32>()]
                    .CharacterCounts[RealmHandle{ fields[3].Get<uint8>(), fields[4].Get<uint8>(), fields[2].Get<uint32>() }.GetAddress()] = fields[1].Get<uint8>();

            } while (characterCountsResult->NextRow());
        }

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_LAST_PLAYER_CHARACTERS);
        stmt->SetData(0, accountInfo->Id);
        if (PreparedQueryResult lastPlayerCharactersResult = LoginDatabase.Query(stmt))
        {
            Field* fields = lastPlayerCharactersResult->Fetch();
            RealmHandle realmId{ fields[1].Get<uint8>(), fields[2].Get<uint8>(), fields[3].Get<uint32>() };
            Battlenet::Session::LastPlayedCharacterInfo& lastPlayedCharacter = accountInfo->GameAccounts[fields[0].Get<uint32>()]
                .LastPlayedCharacters[realmId.GetSubRegionAddress()];

            lastPlayedCharacter.RealmId = realmId;
            lastPlayedCharacter.CharacterName = fields[4].Get<std::string>();
            lastPlayedCharacter.CharacterGUID = fields[5].Get<uint64>();
            lastPlayedCharacter.LastPlayedTime = fields[6].Get<uint32>();
        }

        BigNumber ticket;
        ticket.SetRand(20 * 8);

        loginResult.set_login_ticket("AC-" + ByteArrayToHexStr(ticket.ToByteArray<20>()));

        AddLoginTicket(loginResult.login_ticket(), std::move(accountInfo));
    }

    loginResult.set_authentication_state(Battlenet::JSON::Login::DONE);
    return SendResponse(soapClient, loginResult);
}

int32 LoginRESTService::SendResponse(soap* soapClient, google::protobuf::Message const& response)
{
    std::string jsonResponse = JSON::Serialize(response);

    soap_response(soapClient, SOAP_FILE);
    soap_send_raw(soapClient, jsonResponse.c_str(), jsonResponse.length());
    return soap_end_send(soapClient);
}

std::string LoginRESTService::CalculateShaPassHash(std::string const& name, std::string const& password)
{
    Acore::Crypto::SHA256::Digest email = Acore::Crypto::SHA256::GetDigestOf(name);

    Acore::Crypto::SHA256 sha;
    sha.UpdateData(ByteArrayToHexStr(email));
    sha.UpdateData(":");
    sha.UpdateData(password);
    sha.Finalize();

    return ByteArrayToHexStr(sha.GetDigest(), true);
}

std::unique_ptr<Battlenet::Session::AccountInfo> LoginRESTService::VerifyLoginTicket(std::string const& id)
{
    std::unique_lock<std::mutex> lock(_loginTicketMutex);

    auto itr = _validLoginTickets.find(id);
    if (itr != _validLoginTickets.end())
    {
        if (itr->second.ExpiryTime > time(nullptr))
        {
            std::unique_ptr<Battlenet::Session::AccountInfo> accountInfo = std::move(itr->second.Account);
            _validLoginTickets.erase(itr);
            return accountInfo;
        }
    }

    return std::unique_ptr<Battlenet::Session::AccountInfo>();
}

void LoginRESTService::AddLoginTicket(std::string const& id, std::unique_ptr<Battlenet::Session::AccountInfo> accountInfo)
{
    std::unique_lock<std::mutex> lock(_loginTicketMutex);

    _validLoginTickets[id] = { id, std::move(accountInfo), time(nullptr) + 10 };
}

void LoginRESTService::CleanupLoginTickets(boost::system::error_code const& error)
{
    if (error)
        return;

    time_t now = time(nullptr);

    {
        std::unique_lock<std::mutex> lock(_loginTicketMutex);
        for (auto itr = _validLoginTickets.begin(); itr != _validLoginTickets.end();)
        {
            if (itr->second.ExpiryTime < now)
                itr = _validLoginTickets.erase(itr);
            else
                ++itr;
        }
    }

    _loginTicketCleanupTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(10));
    _loginTicketCleanupTimer->async_wait(std::bind(&LoginRESTService::CleanupLoginTickets, this, std::placeholders::_1));
}

LoginRESTService::LoginTicket& LoginRESTService::LoginTicket::operator=(LoginTicket&& right)
{
    if (this != &right)
    {
        Id = std::move(right.Id);
        Account = std::move(right.Account);
        ExpiryTime = right.ExpiryTime;
    }

    return *this;
}

Namespace namespaces[] =
{
    { NULL, NULL, NULL, NULL }
};

LoginRESTService& LoginRESTService::Instance()
{
    static LoginRESTService instance;
    return instance;
}

char const* const LoginRESTService::ResponseCodePlugin::PluginId = "bnet-error-code";

int32 LoginRESTService::ResponseCodePlugin::Init(soap* s, soap_plugin* p, void* /*arg*/)
{
    ResponseCodePlugin* data = new ResponseCodePlugin();
    data->fresponse = s->fresponse;

    p->id = PluginId;
    p->fdelete = &Destroy;
    p->data = data;

    s->fresponse = &ChangeResponse;
    return SOAP_OK;
}

void LoginRESTService::ResponseCodePlugin::Destroy(soap* s, soap_plugin* p)
{
    ResponseCodePlugin* data = reinterpret_cast<ResponseCodePlugin*>(p->data);
    s->fresponse = data->fresponse;
    delete data;
}

int32 LoginRESTService::ResponseCodePlugin::ChangeResponse(soap* s, int32 originalResponse, size_t contentLength)
{
    ResponseCodePlugin* self = reinterpret_cast<ResponseCodePlugin*>(soap_lookup_plugin(s, PluginId));
    return self->fresponse(s, self->ErrorCode && originalResponse == SOAP_FILE ? self->ErrorCode : originalResponse, contentLength);
}

char const* const LoginRESTService::ContentTypePlugin::PluginId = "bnet-content-type";

int32 LoginRESTService::ContentTypePlugin::Init(soap* s, soap_plugin* p, void* arg)
{
    ContentTypePlugin* data = new ContentTypePlugin();
    data->fposthdr = s->fposthdr;
    data->ContentType = reinterpret_cast<char const*>(arg);

    p->id = PluginId;
    p->fdelete = &Destroy;
    p->data = data;

    s->fposthdr = &OnSetHeader;
    return SOAP_OK;
}

void LoginRESTService::ContentTypePlugin::Destroy(soap* s, soap_plugin* p)
{
    ContentTypePlugin* data = reinterpret_cast<ContentTypePlugin*>(p->data);
    s->fposthdr = data->fposthdr;
    delete data;
}

int32 LoginRESTService::ContentTypePlugin::OnSetHeader(soap* s, char const* key, char const* value)
{
    ContentTypePlugin* self = reinterpret_cast<ContentTypePlugin*>(soap_lookup_plugin(s, PluginId));
    if (key && !strcmp("Content-Type", key))
        value = self->ContentType;

    return self->fposthdr(s, key, value);
}
