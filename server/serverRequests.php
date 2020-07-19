<?php
require_once 'functions.php';
require_once 'dbconfig.php';

$action = filter_input(INPUT_POST, 'action');

switch ($action) {
    case 'thermSendTemp':
	$temp = filter_input(INPUT_POST,'temp');
	$mac = filter_input(INPUT_POST,'mac');
        echo thermSendTemp($temp,$mac);
        break;
    case 'thermRequestTemp':
        $mac = filter_input(INPUT_POST,'mac');
        echo thermRequestTemp($mac);
        break;
    case 'confirmSmartConfigCompleted':
	    echo confirmSmartConfigCompleted();
	    break;
    case 'checkIfVentsNeedPairing':
        echo checkIfVentNeedsPairing();
        break;
        case 'addNewVent':
            $mac = filter_input(INPUT_POST,'mac');
        echo addNewVent($mac);
        break;
    case 'updateExistingVent':
        $id = filter_input(INPUT_POST,'id');
        $temp = filter_input(INPUT_POST,'temp');
        $status = filter_input(INPUT_POST,'status');
        echo updateExistingVent($id,$temp, $status);
        break;
        case 'getVentData':
        echo getRoomData();
        break;
    case 'checkIfVentCompletedPairing':
        echo checkIfVentCompletedPairing();
        break;
    case 'getIdForMac':
        $mac = filter_input(INPUT_POST,'mac');
        echo getIdForMac($mac);
        break;
    case 'updateHVAC':
        $hvac = filter_input(INPUT_POST,'hvac');
        echo updateHvacStatus($hvac);
        break;
        case 'updateFan':
        $fan = filter_input(INPUT_POST,'fan');
        echo updateFanStatus($fan);
        break;
    default:
        echo null;
        break;
}
