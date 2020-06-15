<?php
    require_once 'dbconfig.php';

    global $mysqli;

    $query = "SELECT * FROM temp";
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
