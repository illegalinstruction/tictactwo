const 	http = require("http");
const 	host = 'localhost';
const 	port = 8000;

//----------------------------------------------------

const PLYRMNGR_handle_new_connect = function(request, response)
{
	console.log("PLYRMNGR_handle_new_connect");

	const nick = request.headers["nick"];
	const pass = request.headers["pass"];

	console.log(nick);

	// did user supply credentials?
	if ((nick == null) || (pass == null) || (nick == "") || (pass == ""))
	{
		// no - login failure
		response.writeHead(401);
		response.end("login credentials missing from request");
	} 

	// check if app id, player name, password are okay

	// if yes, add player to list of active players,
	// get their stats from the database, and return 
	// a session id with 200 OK

	// else return 'server full' with 429 Too Many
	// Requests

	response.writeHead(200);
	response.end("PLYRMNGR_handle_new_connect() called\n");
};

//----------------------------------------------------

const PLYRMNGR_get_active_players = function(request, response)
{
	console.log("PLYRMNGR_get_active_players");

	// fetch the list of logged-in players, along
	// with their avatar ids and return it in a
	// json blob with 200 OK

	// if the list somehow can't be gotten, 
	// return 500 Internal Server Error

	response.writeHead(200);
	response.end("PLYRMNGR_get_active_players() called");
};

//----------------------------------------------------

const PLYRMNGR_get_chat = function(request, response)
{
	console.log("PLYRMNGR_get_chat()");

	// get the chat messages since the timestamp
	// specified in the request and return them
	// in a json blob with 200 OK

	// if this fails somehow, return 500 Internal 
	// Server Error

	// if we get a timestamp more than 5 minutes
	// ago, we clamp the message limit at 5 minutes
	response.writeHead(200);
	response.end("PLYRMNGR_get_chat() called");
};

//----------------------------------------------------

const request_listener = function(request, response)
{
	var path = request.url.toLowerCase().replace("/","");

	if (path === "get_chat")
	{
		return PLYRMNGR_get_chat(request, response);
	}
	if (path === "get_active_players")
	{
		return PLYRMNGR_get_active_players(request, response);
	}
	if (path === "connect")
	{
		return PLYRMNGR_handle_new_connect(request, response);
	}

	response.writeHead(500);
	response.end("unsupported request");
}

//----------------------------------------------------

const server = http.createServer(request_listener);

server.listen(port, host, () =>
{
	console.log("Server is alive at:    ".concat(host.toString()));
	console.log("And listening on port: ".concat(port.toString()));
});
