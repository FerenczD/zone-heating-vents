<?php
    require_once 'dbconfig.php';

    global $mysqli;

   $json = file_get_contents('php://input');
   $obj = json_decode($json,true);

   $id= $obj['id'];
   $temp = $obj['temp'];

    $query = "UPDATE rooms SET setTemp = ? WHERE id = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("ss",$temp,$id);
    $result = $statement->execute();

    echo json_encode(array(
        'message' => $result
   ));
?>
