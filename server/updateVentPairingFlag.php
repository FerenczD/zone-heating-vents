<?php
require_once 'dbconfig.php';
global $mysqli;

$json = file_get_contents('php://input');
$obj = json_decode($json,true);
$flag= $obj['flag'];

$query = "UPDATE flags SET ventNeedsPairing = '".$flag."'";
$statement = $mysqli->prepare($query);
$result = $statement->execute();

echo json_encode(array(
    'message' => $result
));
?>
