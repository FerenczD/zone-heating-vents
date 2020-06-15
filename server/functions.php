<?php
require_once 'serverRequests.php';
require_once 'dbconfig.php';

function thermRequestTemp($mac){

    global $mysqli;
    $data = array();
    $query = "SELECT temperature FROM temp WHERE mac = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("s",	$mac); 
    $statement->execute();
    $result = $statement->get_result();
    while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }
    return $data[0]['temperature'];
}

function thermSendTemp($temp,$mac) {

    global $mysqli;
    $temp = $temp / 100;
    $status = "fail";
    $query = "UPDATE temp SET temperature = ? WHERE mac = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("ss",$temp,$mac); 
    $result = $statement->execute();
if($result){
    $status = "success";
}
return $status;
}
