<?php
    if (isset($_GET['dir'])){$dir = $_GET['dir'];}  else {$dir='../'; }
    if ($dir=='..') {$dir='../';}
    if ($dir=='.') {$dir='../';}
    if ($dir=='./') {$dir='../';}
    if (isset($_GET['n'])){$new = $_GET['n'];}  else {$new='nil'; }
    $files1 = array_reverse(glob($dir . '*.{mp4,mjpg}', GLOB_BRACE));
	$dirs = glob($dir . '*', GLOB_ONLYDIR);
    $alink = '<a class="file" target="_blank" href="play.php?url=';
    
?>

<!doctype html>
<html>
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
		<title>videos</title>
        <link rel="stylesheet" href="pastas.css">
	</head>
	<body>           
       <div id="pastas">
		    <p>folders in <?php echo $dir.'<br>';?></p>
		   <a class="dir" href="list.php?dir=<? echo dirname($dir) ?>/" style="font-size:32px;">↲</a>
            <?php
                foreach($dirs as $key => $value){
                echo '<a class="dir" href="list.php?dir='.$value.'/">'.basename($value).'</a>'."\n";}
            ?>
        </div>
		<div id="arquivos">
            <p>files</p>
			<?php
			foreach($files1 as $key => $value){
                echo $alink.$value.'">'.basename($value).'</a>'."\n";}
			?>
		</div>		
  </body>
</html>
