<?php
require_once 'dbconfig.php';
global $mysqli;

$json = file_get_contents('php://input');
$obj = json_decode($json,true);

$numVents= (int)$obj['numVents'];
$response = "";
$data = array();

$query = "SELECT numRooms, roomMacs FROM home WHERE id = 0";
$statement = $mysqli->prepare($query);
$statement->execute();
$result = $statement->get_result();
while($row = mysqli_fetch_assoc($result)) {
    $data[] = $row;
}

if($data[0]['numRooms'] > $numVents) {
    $macArray = explode(",",$data[0]['roomMacs']);
    $newMac = $macArray[$data[0]['numRooms'] - 1];
    $response = $newMac;
}
echo json_encode(array(
    'message' => $response
));
?>
