<?php
require_once 'dbconfig.php';

global $mysqli;

$json = file_get_contents('php://input');
$obj = json_decode($json,true);

$name= $obj['name'];
$mac= $obj['mac'];

$query = "INSERT INTO rooms SET name = ?, mac = ?";
$statement = $mysqli->prepare($query);
$statement->bind_param("ss",$name,$mac);
$result = $statement->execute();

$query = "UPDATE flags SET ventCompletedPairing = true";
$statement = $mysqli->prepare($query);
$statement->execute();

echo json_encode(array(
    'message' => $result
));
?>
