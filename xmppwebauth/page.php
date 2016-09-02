<?php
session_cache_limiter('none');
session_start();

$user = getSession('user');

if(is_array($user) && $user['accepted']) {
	echo "Welcome, ".$user['jid']."<br/>";
	echo "<a href='logout.php'>Logout</a>";
} else {
	echo "Please, pass <a href='login.php'>login form</a> first.";
}

function getSession($name) {
	global $_SESSION;
	return(isset($_SESSION[$name]) ? $_SESSION[$name] : "");
}

?>