<?php
//ini_set('display_errors', 1);
//ini_set('display_startup_errors', 1);
ini_set("error_log", "php-error.txt");
error_reporting(E_ALL);
// Telegram function which you can call
$bot = "1415075521:AAxxxxxx";
$chat_id = "249000000";

function telegram($msg,$img,$telegram_bot,$telegram_chat_id) {
        $url='https://api.telegram.org/bot'.$telegram_bot.'/sendMessage';
    	$data=array('chat_id'=>$telegram_chat_id,'text'=>$msg.$img);
        $options=array('http'=>array('method'=>'POST','header'=>"Content-Type:application/x-www-form-urlencoded\r\n",'content'=>http_build_query($data),),);
    	$context=stream_context_create($options);
        $result=file_get_contents($url,false,$context);
        return $result;
}

// dayly routines
function dayly_stuff(){
    $timeNow = get_time("Y-m-d");
    if (file_exists("data_$timeNow") == false){
        rename('data',"data_$timeNow");
        mkdir('data');
    }
    if (file_exists("status_$timeNow.txt") == false){
        rename('status.txt',"status_$timeNow.txt");
        //mkdir('status.txt');
    }
}


function black_pic_detect(){
    $id = 'aqua_0';
	if ( isset($_GET['id']) ){ $id = $_GET['id']; }
    $received = file_get_contents('php://input');
    $darkcount = 0;
    $im = new Imagick();
	if ($im->readImageBlob($received)){
        file_put_contents("status.txt","opened an image from $id\n",FILE_APPEND);
      	$it = $im->getPixelIterator(); 		  /* Get iterator */
      	foreach( $it as $row => $pixels ){      	/* Loop trough pixel rows */
      		foreach ( $pixels as $column => $pixel ){       	/* Loop trough the pixels in the row (columns) */
        		$color = $pixel->getColor();
       			if ( $color['g'] < 30 ){
            		$darkcount++;
            	}
        	}
      	}
    	$it->syncIterator(); /* Sync the iterator, this is important to do on each iteration */
    	$pixcount = (($im->getImageWidth() * $im->getImageHeight()) * 0.9);
        file_put_contents("status.txt","threshold is $pixcount, dark pixel count is $darkcount\n",FILE_APPEND);
    	if ($darkcount > $pixcount) {
     	    file_put_contents("status.txt",$darkcount." > ".$pixcount." | Rejected image from $id\n",FILE_APPEND);
            return true;
    	} else {
        	file_put_contents("status.txt",$darkcount." < ".$pixcount." | Accepted image from $id\n",FILE_APPEND);
            return false;
    	}
    } else {
        file_put_contents("status.txt","could not open an image\n",FILE_APPEND);
    }
}

function get_time($format){
    $tz = 'America/Sao_Paulo';
	$timestamp = time();
	$dt = new DateTime("now", new DateTimeZone($tz)); //first argument "must" be a string
	$dt->setTimestamp($timestamp); //adjust the object to correct timestamp
	return $dt->format($format);
}

function save_photo(){
    $id = 'aqua_0';
	if ( isset($_GET['id']) ){ $id = $_GET['id']; }
    $bot = "1415075521:AAxxxxxx";
	$chat_id = "249000000";
    $received = file_get_contents('php://input');
	//$received_get = $_GET["pic"];
	$timeNow = get_time("Y-j-F.H_i_s");
	$timeNow_ = get_time("Y j F H:i:s\n");
	//echo $timeNow;
	$subject = "$id Mexeu - ".$timeNow."\n";
	//save_photo($received);
	//echo "teste";
	file_put_contents("data/$id.$timeNow.jpg",$received);
	telegram ($subject,"data/$id.$timeNow.jpg",$bot,$chat_id);
}

function save_mjpeg(){
    $id = 'aqua_0';
	if ( isset($_GET['id']) ){ $id = $_GET['id']; }
    $bot = "1415075521:AAxxxxxx";
	$chat_id = "249000000";

	$timeNow = get_time("Y-j-F.H_i_s");
	$timeNow_ = get_time("Y j F H:i:s\n");

    $received = file_get_contents('php://input');
	$aviheader= substr($received, -240);
	$avibody = substr($received, 0, -240);
	$avifile = $aviheader.$avibody;
	file_put_contents("data/$timeNow.$id.avi", $avifile);

	shell_exec("nohup ffmpeg -i data/$timeNow.$id.avi -c:v libx264 -preset slow -profile:v main -vf format=yuv420p -b:v 2000k data/z.$timeNow.$id.mp4 && rm data/$timeNow.$id.avi");

	//send_file_to_apache_dumb_server("data/z.$timeNow.$id.mp4");

	$subject = "$id Mexeu - ".$timeNow."\n";
	telegram($subject,"http://yoursite.com/list.php",$bot,$chat_id);
}

function send_file_to_apache_dumb_server($the_file){
	$timeNow_ = get_time("Y j F h:i:s\n");
	// set up basic connection
	$ftp_server = "server addr";
	$conn_id = ftp_connect($ftp_server); 

	// login with username and password
	$ftp_user_name = "ftp user";
	$ftp_user_pass = "ftp pass";
	$login_result = ftp_login($conn_id, $ftp_user_name, $ftp_user_pass); 

	// check connection
	if ((!$conn_id) || (!$login_result)) { 
		file_put_contents("status.txt",$timeNow_." --> FTP connection has failed!\n",FILE_APPEND);
		file_put_contents("status.txt",$timeNow_." --> Attempted to connect to $ftp_server for user $ftp_user_name\n",FILE_APPEND);
		exit; 
	} else {
		file_put_contents("status.txt",$timeNow_." --> Connected to $ftp_server, for user $ftp_user_name\n",FILE_APPEND);
	}

	// upload the file
	$destination_file = "/aquariuscam/$the_file";
	//$fp = fopen($the_file, 'r');
	$upload = ftp_put($conn_id, $destination_file, $the_file, FTP_BINARY); 

	// check upload status
	if (!$upload) { 
		file_put_contents("status.txt",$timeNow_." --> FTP upload has failed!\n",FILE_APPEND);
	} else {
		file_put_contents("status.txt",$timeNow_." --> Uploaded $the_file to $ftp_server as $destination_file\n",FILE_APPEND);
	}

	// close the FTP stream 
	ftp_close($conn_id); 
	//fclose($fp);
}

function save_motion(){
    $id = 'aqua_0';
	if ( isset($_GET['id']) ){ $id = $_GET['id']; }
	$received = file_get_contents('php://input');
	$timeNow = get_time("Y-j-F.H_i_s");
	$timeNow_ = get_time("Y j F H:i:s\n");
	file_put_contents("data/$id.motion.".$timeNow.'.txt',$received);
}

function save_bg(){
    $id = 'aqua_0';
	if ( isset($_GET['id']) ){ $id = $_GET['id']; }
	$received = file_get_contents('php://input');
	$timeNow = get_time("Y-j-F.H_i_s");
	$timeNow_ = get_time("Y j F H:i:s\n");
	file_put_contents("data/$id.bg.".$timeNow.'.txt',$received);
}

function save_status(){
    $id = 'aqua_0';
	if ( isset($_GET['id']) ){ $id = $_GET['id']; }
	$received = file_get_contents('php://input');
	$timeNow_ = get_time("Y j F H:i:s\n");
	file_put_contents("status.txt",$timeNow_.' --> '.$id.$received."\n",FILE_APPEND);
	$timeNow__ = get_time("Y-m-d\TH:i:sP");
	file_put_contents("cam_status/".$id.".txt",$timeNow__);
}

function send_orders(){
    $config = "SLP";
    $config = file_get_contents("current_orders.txt",false);
	echo $config;
}
function send_led(){
    $led = "NLE";
    $led = file_get_contents("led.txt",false);
	echo $led;
}

dayly_stuff();

if ($_GET['pic'] == 'motion_detect'){
    if (black_pic_detect() == false){
      save_photo();
    }
    //save_photo();
}
if ($_GET['pic'] == 'mjpeg'){
    //if (black_pic_detect() == false){
      save_mjpeg();
    //}
    //save_photo();
}
if ($_GET['pic'] == 'status'){
    save_status();
}

if ($_GET['pic'] == 'orders'){
    send_orders();
}

if ($_GET['pic'] == 'motion_debug'){
    save_motion();
}

if ($_GET['pic'] == 'motion_debug_bg'){
    save_bg();
}
if ($_GET['pic'] == 'led'){
    send_led();
}

?>