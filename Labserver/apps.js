var panel = require('./build/Release/panel.node');
var express = require('express');
var app = express();

var server = require('http').createServer(app);
var io = require('socket.io')(server);
var port =  80;

app.use(express.static(__dirname + '/public'));

server.listen(port, function () {
  console.log('Server listening at port %d', port);
  
});


var configured = false, authenticated = false;

var lastrpmdata= -1;

function sendMessage(socket){  
	if( lastrpmdata != panel.rpm()){
		var data = {};
		data.rpm = panel.rpm();
		socket.emit("refresh", data);
	}
}
  
  
io.on('connection', function (socket) {
	var interval;

  socket.on('new connection', function(data){
	
	hold = false;
	if ((configured = panel.setup()) ==1){
		panel.run();
		sendMessage(socket);
	}
	
  });
  
  
 socket.on('motor', function(data){
	 console.log("motor: " + data.motor);
	 panel.motor(parseInt(data.motor));
	
 });
 

  
socket.on('lamp', function (data) {
	console.log("lamp: " + data.sw);
	
	
	if(data.sw){
		panel.update("1");
		interval = setInterval(function (){
			sendMessage(socket);
	    },1000);
	  
	}else{
		clearInterval(interval);
		panel.update("0");
		setTimeout(function(){
			sendMessage(socket);
		},2000);
	}
	
	
	
});



 socket.on('disconnect', function () {
	console.log('disconnected');
	
	
	console.log('disconnected');
	if (configured) {
		clearInterval(interval);
	    panel.update("0");
		configured = authenticated = false;
		panel.exit();
	}else{
		panel.setup();
		panel.run();
		panel.exit();
	}
	console.log('disconnected');

  });
  
  
});