<?php
require_once 'dbconfig.php';
global $mysqli;


$query = "UPDATE flags SET ventCompletedPairing = false";
$statement = $mysqli->prepare($query);
$result = $statement->execute();

echo json_encode(array(
    'message' => $result
));
?>
