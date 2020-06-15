<?php
    require_once 'dbconfig.php';

    global $mysqli;


    $json = file_get_contents('php://input');
   $obj = json_decode($json,true);

   $temp= $obj['temp'];
   $room = $obj['room'];

    $query = "UPDATE temp SET temperature = ? WHERE name = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("ss",$temp,$room);
    $result = $statement->execute();

    echo json_encode(array(
        'message' => $result
   ));
?>