<?php
require_once 'dbconfig.php';
global $mysqli;

$json = file_get_contents('php://input');

$query = "SELECT * FROM rooms";
$data = array();
$statement = $mysqli->prepare($query);
$statement->execute();
$result = $statement->get_result();
while($row = mysqli_fetch_assoc($result)) {
    $data[] = $row;
}
echo json_encode(array(
    'message' => $data
));
?>
