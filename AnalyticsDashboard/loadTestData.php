<?php

    /*
    * Project created by Numaan Chaudhry 29/11/2016
    * Demo Instructable for #IntelIOT
    * This script loads dummy data into the database.
    * *IMPORTANT* - this will clear ALL EXISTING DATA. Use with Caution.
    */

    require("common.php");


    $cursor = $sensorCollection->find();
    foreach ($cursor as $document)
        $sensorCollection->remove(array("_id"=>new MongoId($document["_id"])));
        
    $cursor = $staticCollection->find();
    foreach ($cursor as $document)
        $staticCollection->remove(array("_id"=>new MongoId($document["_id"])));


    $dummySensorData = array(
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_001","BattV":23.45,"PVV":45.3,"Ct":350,"Temp":32}',
              "ModuleID"    =>"sl_001",
              "Location"    =>"Lungi",
              "ServerTime"  =>"2016-05-08 07:44:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_001","BattV":23.55,"PVV":35.3,"Ct":225,"Temp":32}',
              "ModuleID"    =>"sl_001",
              "Location"    =>"Lungi",
              "ServerTime"  =>"2016-05-09 09:44:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_001","BattV":22.9,"PVV":47.3,"Ct":105,"Temp":32}',
              "ModuleID"    =>"sl_001",
              "Location"    =>"Lungi",
              "ServerTime"  =>"2016-05-10 07:44:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_002","BattV":12.7,"PVV":35,"Ct":534,"Temp":29}',
              "ModuleID"    =>"sl_002",
              "Location"    =>"Daru",
              "ServerTime"  =>"2016-05-08 07:44:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_002","BattV":12.6,"PVV":38,"Ct":0,"Temp":31}',
              "ModuleID"    =>"sl_002",
              "Location"    =>"Daru",
              "ServerTime"  =>"2016-05-09 15:11:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_002","BattV":11.9,"PVV":48,"Ct":0,"Temp":28}',
              "ModuleID"    =>"sl_002",
              "Location"    =>"Daru",
              "ServerTime"  =>"2016-05-10 12:33:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_003","BattV":23.45,"PVV":45.3,"Ct":0,"Temp":32}',
              "ModuleID"    =>"sl_003",
              "Location"    =>"Kenema",
              "ServerTime"  =>"2016-05-08 09:15:05",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_003","BattV":24,"PVV":55,"Ct":745,"Temp":32}',
              "ModuleID"    =>"sl_003",
              "Location"    =>"Kenema",
              "ServerTime"  =>"2016-05-09 12:32:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
        array("Source"      =>"WEB",
              "Data"        =>'{"ModuleID":"sl_003","BattV":25,"PVV":60,"Ct":435,"Temp":32}',
              "ModuleID"    =>"sl_003",
              "Location"    =>"Kenema",
              "ServerTime"  =>"2016-05-10 11:18:34",
              "RequestParams"=>"[...]",
              "ServerParams"=>"[...]",
              "Headers"     =>"[...]"
              ),
    );

    $dummyStaticData = array(
        array("Title"   =>"Systems Deployed",
              "Data"    => "302",
              "UIIcon"  =>"tasks"
              ),
        array("Title"   =>"Systems Being Monitored",
              "Data"    => "3",
              "UIIcon"  =>"eye"
              ),
        array("Title"   =>"Issues",
              "Data"    => "1",
              "UIIcon"  =>"warning"
              ),
        array("Title"   =>"Systems Requiring Maintenance",
              "Data"    => "2",
              "UIIcon"  =>"wrench"
              )
    );

    foreach($dummySensorData as $doc)
        $sensorCollection->insert($doc);

    foreach($dummyStaticData as $doc)
        $staticCollection->insert($doc);

    echo "Completed Successfully";
?>