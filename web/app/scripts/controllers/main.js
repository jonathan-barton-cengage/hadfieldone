'use strict';

angular.module('project.hadfield')

  .controller('MainCtrl', function($scope, $location, version) {

    $scope.$path = $location.path.bind($location);
    $scope.version = version;
    $scope.projectTitle = "Hadfield";
    $scope.mainMenuItems = [
        {
            title: "",
            url: "",
            items: []
        },
        {
            title: "",
            url: ""
        },
        {
            title: "",
            url: ""
        }
    ];

        var mainCtrl = {

        };

    return mainCtrl;

  });
