<?php
require_once 'dbconfig.php';

global $mysqli;

$json = file_get_contents('php://input');
$obj = json_decode($json,true);

$name= $obj['name'];

$query = "UPDATE home SET home = ? WHERE id = 0";
$statement = $mysqli->prepare($query);
$statement->bind_param("s",$name);
$result = $statement->execute();

echo json_encode(array(
    'message' => $result
));
?>
