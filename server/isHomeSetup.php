<?php
    require_once 'dbconfig.php';
    global $mysqli;
    $status = false;
    $query = "SELECT * FROM home";
    $data = array();
    $statement = $mysqli->prepare($query);
    $statement->execute();
    $result = $statement->get_result();
    while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }
    if($data[0]['home'] && $data[0]['numRooms'] && $data[0]['roomMacs']) {
	    $status = true;
    }
    echo json_encode(array(
        'message' => $status
   ));
?>
