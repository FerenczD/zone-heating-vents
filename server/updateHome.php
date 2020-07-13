<?php
    require_once 'dbconfig.php';

    global $mysqli;

    $json = file_get_contents('php://input');
   $obj = json_decode($json,true);

   $home = $obj['home'];
   $date = $obj['date'];
   $temp = $obj['temp'];

    $query = "UPDATE home SET date = ?, outsideTemp = ? WHERE home = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("sss",$date,$temp,$home);
    $result = $statement->execute();

    echo json_encode(array(
        'message' => $result
   ));
?>