<?php
session_cache_limiter('none');
session_start();

$queue = "out_testbot";

$login = getParam('login');
$password = getParam('password');

$user = getSession('user');

if(($login==null && $password==null) || ($login!=null && $password!=null)) {
	loginForm(null);
	exit;
}

if($login!=null) {
	passwordForm($queue, $login);
	exit;
}

echo "password = ".$user['password']."<br/>";

if(is_array($user) && $password == $user['password']) {
	$user['accepted'] = true;
	if(isset($user['password'])) {
		unset($user['password']);
	}
	setSession('user', $user);
	header('Location: page.php');
	exit;
}

loginForm("Wrond password!");
exit;

function loginForm($msg) {
	removeSession("user");
	if($msg!=null) {
		echo "<b>".$msg."</b><br/>";
	}
?><form action='login.php' method='post'>
<input type='text' name='login' placeHolder='Jabber ID' /> <input type='submit' value='Login' />
</form>
<?php
}

function passwordForm($queue, $jid) {

	$password = createPassword();

	$user = array();
	$user['jid'] = $jid;
	$user['accepted'] = false;
	$user['password'] = $password;
	setSession('user', $user);

	$msg = "Your password is ".$password;

	$redis = new Redis();
	$redis->pconnect( "127.0.0.1", 6379 );
	$redis->publish( $queue, "jid:" . $jid . "\n" . $msg );

?>Your password has been sent to Your JID<br/>
<form action='login.php' method='post'>
<input type='password' name='password' placeHolder='Password' /> <input type='submit' value='Login' />
</form>
<?php
}

function createPassword() {
	$alphas = 'ABCDEFGHIJKLMOPQRSTUVXWYZ';
	$chars = $alphas.strtolower($alphas).'0123456789_-@$%^&*()';
	$lenChars = strlen($chars) - 1;
	$result = "";
	for($x=0; $x<24; $x++){
		$idx = rand(0, $lenChars);
		$result .= substr($chars, $idx, 1);
	}
	return $result;
}

function getParam($name) {
	global $_POST;
	global $_GET;
	if(isset($_POST[$name])) {
		return trim(stripslashes($_POST[$name]));
	}
	if(isset($_GET[$name])) {
		return trim(stripslashes($_GET[$name]));
	}
	return null;
}

function getSession($name) {
	global $_SESSION;
	return(isset($_SESSION[$name]) ? $_SESSION[$name] : "");
}

function setSession($name,$val) {
	global $_SESSION;
	$_SESSION[$name]=$val;
}

function removeSession($name) {
	global $_SESSION;
	unset($_SESSION[$name]);
}

?>