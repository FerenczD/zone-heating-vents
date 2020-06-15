<?php
    require_once 'dbconfig.php';
    global $mysqli;

    $mac= filter_input(INPUT_POST, 'mac');
    $data = array();
    $query = "SELECT temperature FROM temp WHERE mac = ?";
    $statement = $mysqli->prepare($query);
    $statement->bind_param("s",	$mac); 
    $statement->execute();
 $result = $statement->get_result();
 while($row = mysqli_fetch_assoc($result)) {
        $data[] = $row;
    }

    echo $data[0]['temperature'];
?>	

 
