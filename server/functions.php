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
    $status = 0;
    $query = "UPDATE temp SET temperature = ? WHERE mac = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("ss",$temp,$mac);
    $result = $statement->execute();
if($result){
    $status = 1;
}
return $status;
}

function confirmSmartConfigCompleted() {
    global $mysqli;
    $status = 0;
    $query = "UPDATE home SET paired = true";
    $statement = $mysqli->prepare($query);
    $result = $statement->execute();
    if($result){
        $status = 1;
    }
    return $status;
}

function checkIfVentNeedsPairing() {
    global $mysqli;
    $data = array();
    $query = "SELECT ventNeedsPairing FROM flags WHERE 1";
    $statement = $mysqli->prepare($query);
    $statement->execute();
    $result = $statement->get_result();
    while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }
    return $data[0]['ventNeedsPairing'];
}

function checkIfVentCompletedPairing() {
    global $mysqli;
    $data = array();
    $query = "SELECT ventCompletedPairing FROM flags WHERE 1";
    $statement = $mysqli->prepare($query);
    $statement->execute();
    $result = $statement->get_result();
    while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }
    return $data[0]['ventCompletedPairing'];
}


function addNewVent($mac) {
    global $mysqli;
   $allVents = getAllVents();
   $allVentsArray = explode(",",$allVents);
   array_push($allVentsArray, $mac);
   if($allVentsArray[0] == "") {
       array_shift($allVentsArray);
   }
   $numOfVents = count($allVentsArray);
   $allVents = implode(",",$allVentsArray);
    $status = 0;

    $query = "UPDATE home SET roomMacs = ?,numRooms = ? WHERE id = 0";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("si",$allVents,$numOfVents);
    $result = $statement->execute();
    updateVentPairingFlag(false);
    return $result;
}

function getIdForMac($mac) {
    global $mysqli;
    $data = array();

    $query = "SELECT id FROM rooms WHERE MAC = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("s",$mac);
    $statement->execute();
    $result = $statement->get_result();
    while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }

    $query = "UPDATE flags SET ventCompletedPairing = false";
    $statement = $mysqli->prepare($query);
    $statement->execute();

    return $data[0]['id'];
}

function getAllVents() {

    global $mysqli;
    $data = array();
    $query = "SELECT roomMacs FROM home WHERE id = 0";
    $statement = $mysqli->prepare($query);
    $statement->execute();
    $result = $statement->get_result();
    while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }
    return $data[0]['roomMacs'];
}

function updateVentPairingFlag($flag) {
    global $mysqli;

    $query = "UPDATE flags SET ventNeedsPairing = '".$flag."'";
    $statement = $mysqli->prepare($query);
    $statement->execute();
    return $statement->get_result();
}

function updateExistingVent($id,$temp, $status) {
    global $mysqli;
    $success = 0;
    $temp = $temp / 100;
    $query = "UPDATE rooms SET temperature = ?, status = ? WHERE id = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("sss",$temp,$status, $id);
    $result = $statement->execute();
    if($result){
        $success = 1;
    }

    return $success;
}

function getRoomData() {
    global $mysqli;
    $data = array();
    $query = "SELECT * FROM rooms";
    $statement = $mysqli->prepare($query);
    $statement->execute();
    $result = $statement->get_result();
    while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }
    if(count($data) < 1) {
        return "0";
    } else {
        return json_encode($data);
    }

}

function updateHvacStatus($hvac) {

    global $mysqli;
    $status = 0;
    $query = "UPDATE home SET hvac = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("s",$hvac);
    $result = $statement->execute();
    if($result){
        $status = 1;
    }
    return $status;
}

function updateFanStatus($fan) {

    global $mysqli;
    $status = 0;
    $query = "UPDATE home SET fan = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("s",$fan);
    $result = $statement->execute();
    if($result){
        $status = 1;
    }
    return $status;

}
