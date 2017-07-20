var express = require('express');
var panel = require('./build/Release/panel.node');


var app = express();
var configured = true;
var pos =1000;
app.get('/:command', function (req, res) {

console.log(req.params.command);


if(req.params.command == 'setup'){
	configured = panel.setup();
	res.send("1 " + configured);
	
}else if(req.params.command == 'run'){
	if (configured)
		panel.run();

    res.send("OK!");

}else if(req.params.command == 'exit'){
	if(configured){
		panel.exit();
		configured = false;
	}
	res.send("OK!");

}else if(req.params.command == 'motor'){
	if(configured){
		panel.motor(pos);
                pos+=100;
		res.send("OK");
	}else{
		res.send("NOK!");
	}

}else if(req.params.command == 'on'){
	if(configured){
		panel.update("1");

		res.send("OK!");
	}

}else if(req.params.command == 'off'){
	if(configured){
		panel.update("0");

		res.send("OK!");
	}

}else{
	if(configured && !isNaN(req.params.command)){
		panel.switch(1,parseInt(req.params.command));
	    res.send("OK");
	}else{
	    res.send("NOK!");

	}
}
	

});





var server = app.listen(80, function () {

  var host = server.address().address;
  var port = server.address().port;
  
  console.log('App listening at http://%s:%s', host, port);
  panel.setup();
  panel.run();
});

