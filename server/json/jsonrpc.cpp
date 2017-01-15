/***
    This file is part of snapcast
    Copyright (C) 2014-2016  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#include "jsonrpc.h"
#include "common/log.h"


using namespace std;

namespace jsonrpc
{

Request::Request() : method(""), id()
{
}


void Request::parse(const std::string& json)
{
	// http://www.jsonrpc.org/specification
	//	code	message	meaning
	//	-32700	Parse error	Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.
	//	-32600	Invalid Request	The JSON sent is not a valid Request object.
	//	-32601	Method not found	The method does not exist / is not available.
	//	-32602	Invalid params	Invalid method parameter(s).
	//	-32603	Internal error	Internal JSON-RPC error.
	//	-32000 to -32099	Server error	Reserved for implementation-defined server-errors.
	try
	{
		try
		{
			json_ = Json::parse(json);
		}
		catch (const exception& e)
		{
			throw RequestException(e.what(), -32700);
		}

		if (json_.count("id") == 0)
			throw InvalidRequestException("id is missing");

		try
		{
			id = Id(json_["id"]);
		}
		catch(const std::exception& e)
		{
			throw InvalidRequestException(e.what());
		}

		if (json_.count("jsonrpc") == 0)
			throw InvalidRequestException("jsonrpc is missing", id);
		string jsonrpc = json_["jsonrpc"].get<string>();
		if (jsonrpc != "2.0")
			throw InvalidRequestException("invalid jsonrpc value: " + jsonrpc, id);

		if (json_.count("method") == 0)
			throw InvalidRequestException("method is missing", id);
		method = json_["method"].get<string>();
		if (method.empty())
			throw InvalidRequestException("method must not be empty", id);

		params.clear();
		try
		{
			if (json_["params"] != nullptr)
			{
				Json p = json_["params"];
				for (Json::iterator it = p.begin(); it != p.end(); ++it)
					params[it.key()] = it.value();
			}
		}
		catch (const exception& e)
		{
			throw InvalidParamsException(e.what(), id);
		}
	}
	catch (const RequestException& e)
	{
		throw;
	}
	catch (const exception& e)
	{
		throw InternalErrorException(e.what(), id);
	}
}


Json Request::getResponse(const Json& result)
{
	Json response = {
		{"jsonrpc", "2.0"},
		{"id", id.to_json()},
		{"result", result}
	};

	return response;
}


Json Request::getError(int code, const std::string& message)
{
	Json response = {
		{"jsonrpc", "2.0"},
		{"error", {
			{"code", code},
			{"message", message}
		}},
	};

	return response;

}


bool Request::hasParam(const std::string& key)
{
	return (params.find(key) != params.end());
}


Json Request::getParam(const std::string& key)
{
	if (!hasParam(key))
		throw InvalidParamsException(id);
	return params[key];
}



/*
bool JsonRequest::isParam(size_t idx, const std::string& param)
{
	if (idx >= params.size())
		throw InvalidParamsException(*this);
	return (params[idx] == param);
}
*/

Json Notification::getJson(const std::string& method, const Json& data)
{
	Json notification = {
		{"jsonrpc", "2.0"},
		{"method", method},
		{"params", data}
	};

	return notification;
}

}


