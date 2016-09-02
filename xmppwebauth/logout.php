<?php
session_cache_limiter('none');
session_start();

removeSession("user");

header('Location: /page.php');

exit;

function removeSession($name) {
	global $_SESSION;
	unset($_SESSION[$name]);
}

?>