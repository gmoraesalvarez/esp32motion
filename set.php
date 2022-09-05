<!DOCTYPE html>
<html>
    <head>
        <title>cam</title>
        <meta http-equiv="content-type" content="text/html;charset=utf-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
        <link href="corpo.css?v=0.50.51" rel="stylesheet" type="text/css" >
    </head>
    <?
    	$id = $_GET['id'];
		$config = file_get_contents("current_orders.txt",false);
        $files1 = glob("cam_status/" . '*.{txt,cfg}', GLOB_BRACE);
    	$order = substr($config,0,3);
    	$led = substr($config,4,3);
      	$BGLIM = substr($config,8,3);//echo $SPCNT;
    	$BLDIF = substr($config,12,3);
    	$IMDIF = substr($config,16,3);
		echo "<script>order = \"$order\" ;led = \"$led\" ;BGLIM = \"$BGLIM\" ;BLDIF = \"$BLDIF\" ;IMDIF = \"$IMDIF\" ;</script>";
	?>
    <body>
        <?
        foreach($files1 as $cam){
            $bg = 'hsl(128, 40%, 24%)';
            $info = file_get_contents($cam);
            if (time() > strtotime($info)+360) {
                $bg = 'hsl(0, 40%, 24%)';
            }
            $info = substr($info,11,8);
            $id_ = substr($cam,11,12);
            echo "<div class='info' style='background:$bg;'> $id_<br>$info</div>";
        }
        ?>
        <br><br>
        <a href="javascript:void(0)" onclick="set_orders(0)" id="slp">desativar</a>
        <a href="javascript:void(0)" onclick="set_orders(1)" id="mdt">detectar movimento</a>
        <a href="javascript:void(0)" onclick="set_orders(2)" id="pic">enviar fotos</a>
        <a href="javascript:void(0)" onclick="set_led()" id="led">LUZ</a>
        <div class="configs" id="configs">
            <div>
                BG Timer Limit
                <a href="javascript:void(0)" onclick="set_configs('BGLIM',02)" id="s12">2s</a>
            	<a href="javascript:void(0)" onclick="set_configs('BGLIM',10)" id="s48">10s</a>
                <a href="javascript:void(0)" onclick="set_configs('BGLIM',30)" id="s96">30s</a>
            </div>
            <div>
                Block Diff Thresh
                <a href="javascript:void(0)" onclick="set_configs('BLDIF',15)" id="b15">15%</a>
            	<a href="javascript:void(0)" onclick="set_configs('BLDIF',35)" id="b35">35%</a>
            	<a href="javascript:void(0)" onclick="set_configs('BLDIF',50)" id="b50">50%</a>
            </div>
            <div>
                Image Diff Thresh
                <a href="javascript:void(0)" onclick="set_configs('IMDIF',5)" id="i05">05%</a>
            	<a href="javascript:void(0)" onclick="set_configs('IMDIF',15)" id="i15">15%</a>
            	<a href="javascript:void(0)" onclick="set_configs('IMDIF',30)" id="i30">30%</a>
            </div>
        </div>
    </body>
    <script>
    abg = '#545454';
    acolor = '#fafafa';
    pressbg = '#868686';
    presscolor = '#F9A825';
    switch (order){
        case "SLP":
        	document.getElementById('slp').style.background = pressbg;
        	document.getElementById('slp').style.color = presscolor;
    		break;
        case "MDT":
        	document.getElementById('mdt').style.background = pressbg;
        	document.getElementById('mdt').style.color = presscolor;
			break;
        case "PIC":
        	document.getElementById('pic').style.background = pressbg;
        	document.getElementById('pic').style.color = presscolor;
            break;
    }
    if (led == "YLE"){
        document.getElementById('led').style.background = pressbg;
        document.getElementById('led').style.color = presscolor;
    }
    switch (BGLIM){
        case "002":
        	document.getElementById('s12').style.background = pressbg;
        	document.getElementById('s12').style.color = presscolor;
    		break;
        case "010":
        	document.getElementById('s48').style.background = pressbg;
        	document.getElementById('s48').style.color = presscolor;
    		break;
        case "030":
        	document.getElementById('s96').style.background = pressbg;
        	document.getElementById('s96').style.color = presscolor;
    		break;    
    }
    switch (BLDIF){
        case "015":
        	document.getElementById('b15').style.background = pressbg;
        	document.getElementById('b15').style.color = presscolor;
    		break;
        case "035":
        	document.getElementById('b35').style.background = pressbg;
        	document.getElementById('b35').style.color = presscolor;
    		break;
        case "050":
        	document.getElementById('b50').style.background = pressbg;
        	document.getElementById('b50').style.color = presscolor;
    		break;    
    }
    switch (IMDIF){
        case "005":
        	document.getElementById('i05').style.background = pressbg;
        	document.getElementById('i05').style.color = presscolor;
    		break;
        case "015":
        	document.getElementById('i15').style.background = pressbg;
        	document.getElementById('i15').style.color = presscolor;
    		break;
        case "030":
        	document.getElementById('i30').style.background = pressbg;
        	document.getElementById('i30').style.color = presscolor;
    		break;    
    }
    
   	function set_orders(ordern){
        document.getElementById('slp').style.background = abg;
        document.getElementById('slp').style.color = acolor;
        document.getElementById('mdt').style.background = abg;
        document.getElementById('mdt').style.color = acolor;
        document.getElementById('pic').style.background = abg;
        document.getElementById('pic').style.color = acolor;
        if (ordern == 0){
            order = "SLP";
            document.getElementById('slp').style.background = pressbg;
        	document.getElementById('slp').style.color = presscolor;
        }
        if (ordern == 1){
            order = "MDT";
            document.getElementById('mdt').style.background = pressbg;
        	document.getElementById('mdt').style.color = presscolor;
        }
        if (ordern == 2){
            order = "PIC";
            document.getElementById('pic').style.background = pressbg;
        	document.getElementById('pic').style.color = presscolor;
        }
        salvar(order+' '+led+' '+BGLIM+' '+BLDIF+' '+IMDIF+' CTRL_STRING','current_orders');
    }
        
    function set_led(){
		if (led == 'YLE'){
        	led = 'NLE';
            document.getElementById('led').style.background = abg;
        	document.getElementById('led').style.color = acolor;
        }
        else {
            led = 'YLE';
            document.getElementById('led').style.background = pressbg;
        	document.getElementById('led').style.color = presscolor;
        }
        salvar(order+' '+led+' '+BGLIM+' '+BLDIF+' '+IMDIF+' CTRL_STRING','current_orders');
    }
        
    function set_configs(cfg,val){
        switch (cfg){
        	case "BGLIM":
                document.getElementById('s12').style.background = abg;
        		document.getElementById('s12').style.color = acolor;
                document.getElementById('s48').style.background = abg;
        		document.getElementById('s48').style.color = acolor;
                document.getElementById('s96').style.background = abg;
        		document.getElementById('s96').style.color = acolor;
            	switch (val){
            	    case 02:
                		document.getElementById('s12').style.background = pressbg;
        				document.getElementById('s12').style.color = presscolor;
                        BGLIM = "002";
                        break;
                    case 10:
                		document.getElementById('s48').style.background = pressbg;
        				document.getElementById('s48').style.color = presscolor;
                        BGLIM = "010";
                        break;
                    case 30:
                		document.getElementById('s96').style.background = pressbg;
        				document.getElementById('s96').style.color = presscolor;
                        BGLIM = "030";
                        break;
            	}
    		break;
        case "BLDIF":
			    document.getElementById('b15').style.background = abg;
        		document.getElementById('b15').style.color = acolor;
                document.getElementById('b35').style.background = abg;
        		document.getElementById('b35').style.color = acolor;
                document.getElementById('b50').style.background = abg;
        		document.getElementById('b50').style.color = acolor;
            	switch (val){
            	    case 15:
                		document.getElementById('b15').style.background = pressbg;
        				document.getElementById('b15').style.color = presscolor;
                        BLDIF = "015";
                        break;
                    case 35:
                		document.getElementById('b35').style.background = pressbg;
        				document.getElementById('b35').style.color = presscolor;
                        BLDIF = "035";
                        break;
                    case 50:
                		document.getElementById('b50').style.background = pressbg;
        				document.getElementById('b50').style.color = presscolor;
                        BLDIF = "050";
                        break;
            	}
    		break;
        case "IMDIF":
				document.getElementById('i05').style.background = abg;
        		document.getElementById('i05').style.color = acolor;
                document.getElementById('i15').style.background = abg;
        		document.getElementById('i15').style.color = acolor;
                document.getElementById('i30').style.background = abg;
        		document.getElementById('i30').style.color = acolor;
            	switch (val){
            	    case 5:
                		document.getElementById('i05').style.background = pressbg;
        				document.getElementById('i05').style.color = presscolor;
                        IMDIF = "005";
                        break;
                    case 15:
                		document.getElementById('i15').style.background = pressbg;
        				document.getElementById('i15').style.color = presscolor;
                        IMDIF = "015";
                        break;
                    case 30:
                		document.getElementById('i30').style.background = pressbg;
        				document.getElementById('i30').style.color = presscolor;
                        IMDIF = "030";
                        break;
            	}
    		break;    
   		}
        salvar(order+' '+led+' '+BGLIM+' '+BLDIF+' '+IMDIF+' CTRL_STRING','current_orders');
    }
    function salvar(data,nome){
        console.log('salvar');
        server = 'https://'+'<? echo $_SERVER['HTTP_HOST'] . substr($_SERVER['REQUEST_URI'],0,strrpos($_SERVER['REQUEST_URI'],'/')); ?>'+'/';
        ////////// SEND DADOS TO SERVER
        //nome = 'current_orders';
    	saveremote = new XMLHttpRequest();
    	saveremote.open("POST", server+'save.php', true);
    	saveremote.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    	saveremote.send('save='+nome+'&dados='+data);
    	console.log('sent: '+data);
    	console.log('going for the save');
    	saveremote.onload = function(){
    		remotedebug=saveremote.responseText;
    		console.log('save feedback:\n '+remotedebug);
        }
    }
    </script>
</html>
