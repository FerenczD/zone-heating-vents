<?php
require_once 'dbconfig.php';

global $mysqli;

$json = file_get_contents('php://input');
$obj = json_decode($json,true);

$name= $obj['name'];
$mac= $obj['mac'];
$defaultTemp = 20;

$query = "INSERT INTO rooms SET name = ?, mac = ?, setTemp = ?";
$statement = $mysqli->prepare($query);
$statement->bind_param("ssi",$name,$mac,$defaultTemp);
$result = $statement->execute();

$query = "UPDATE flags SET ventCompletedPairing = true";
$statement = $mysqli->prepare($query);
$statement->execute();

echo json_encode(array(
    'message' => $result
));
?>
