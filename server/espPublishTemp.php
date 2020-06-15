<?php
    require_once 'dbconfig.php';
    global $mysqli;

   $temp = filter_input(INPUT_POST, 'temp') / 100;
   $mac= filter_input(INPUT_POST, 'mac');

    $query = "UPDATE temp SET temperature = ? WHERE mac = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("ss",$temp,$mac); 
    $result = $statement->execute();
    echo $result;
?>	