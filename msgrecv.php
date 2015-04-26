<?php
/*
 You will need your own sql_login.php file that includes $db_username, $db_password, $db_hostname, and $db_database.
*/
require_once 'sql_login.php';
	
$reqIndex = 0;
if (isset($_GET['index'])) {
	if (is_numeric($_GET['index'])) {
		$reqIndex = $_GET['index'];
	}
}

try {
        // Establish a connection to the SQL database
        $server = new PDO("mysql:host=$db_hostname;dbname=$db_database;charset=utf8", "$db_username", "$db_password");
        $server->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
} catch (PDOException $e) {
        $error = "Failed to connect to the database.";
}
	
$stmt = $server->query("SELECT * FROM messages ORDER BY date ASC");
$rows = $stmt->fetchAll();
$rowCount = $stmt->rowCount();
if ($reqIndex < $rowCount && $reqIndex >= 1) {
	$index = $reqIndex;
} else {
	$index = $rowCount;
}

// This is the format of the message that msnger.ino understands when parsing messages
echo "%^" . $index . "/" . $rowCount . "^" . $rows[$index-1][1] . ":" . $rows[$index-1][0] . "%";
	
$server = null;
?>
